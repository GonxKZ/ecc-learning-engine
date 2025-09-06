#!/usr/bin/env python3
"""
ECScope Documentation Generator
Automatically generates API documentation from C++ headers and integrates with interactive system
"""

import os
import re
import json
import argparse
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any
from dataclasses import dataclass, field
import subprocess
import xml.etree.ElementTree as ET

@dataclass
class DocComment:
    """Represents a documentation comment"""
    brief: str = ""
    description: str = ""
    params: Dict[str, str] = field(default_factory=dict)
    returns: str = ""
    examples: List[str] = field(default_factory=list)
    see_also: List[str] = field(default_factory=list)
    since: str = ""
    deprecated: str = ""
    notes: List[str] = field(default_factory=list)

@dataclass
class Function:
    """Represents a C++ function or method"""
    name: str
    signature: str
    namespace: str
    class_name: str = ""
    template_params: List[str] = field(default_factory=list)
    parameters: List[Tuple[str, str]] = field(default_factory=list)
    return_type: str = ""
    is_const: bool = False
    is_static: bool = False
    is_virtual: bool = False
    visibility: str = "public"
    doc: DocComment = field(default_factory=DocComment)
    file_location: str = ""
    line_number: int = 0
    examples: List[str] = field(default_factory=list)

@dataclass
class Class:
    """Represents a C++ class or struct"""
    name: str
    namespace: str
    template_params: List[str] = field(default_factory=list)
    base_classes: List[str] = field(default_factory=list)
    methods: List[Function] = field(default_factory=list)
    members: List[Dict] = field(default_factory=list)
    doc: DocComment = field(default_factory=DocComment)
    file_location: str = ""
    line_number: int = 0
    is_template: bool = False
    examples: List[str] = field(default_factory=list)

@dataclass
class APIModule:
    """Represents a module or header file"""
    name: str
    file_path: str
    description: str = ""
    classes: List[Class] = field(default_factory=list)
    functions: List[Function] = field(default_factory=list)
    enums: List[Dict] = field(default_factory=list)
    constants: List[Dict] = field(default_factory=list)
    includes: List[str] = field(default_factory=list)
    examples: List[str] = field(default_factory=list)

class ECScope_DocGenerator:
    """Main documentation generator for ECScope"""
    
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.include_path = self.project_root / "include" / "ecscope"
        self.docs_path = self.project_root / "docs"
        self.interactive_path = self.docs_path / "interactive"
        self.examples_path = self.project_root / "examples"
        
        self.modules: Dict[str, APIModule] = {}
        self.all_classes: Dict[str, Class] = {}
        self.all_functions: Dict[str, List[Function]] = {}
        
        # Configuration
        self.config = {
            "skip_private": True,
            "include_examples": True,
            "generate_diagrams": True,
            "use_doxygen": True,
            "interactive_examples": True
        }

    def generate_all_documentation(self):
        """Generate complete documentation suite"""
        print("🚀 Starting ECScope documentation generation...")
        
        # Step 1: Parse C++ headers
        print("📖 Parsing C++ headers...")
        self.parse_headers()
        
        # Step 2: Extract examples from source
        print("💡 Extracting code examples...")
        self.extract_examples()
        
        # Step 3: Generate API documentation
        print("📚 Generating API documentation...")
        self.generate_api_docs()
        
        # Step 4: Generate interactive tutorials
        print("🎯 Generating interactive tutorials...")
        self.generate_interactive_tutorials()
        
        # Step 5: Generate performance documentation
        print("⚡ Generating performance documentation...")
        self.generate_performance_docs()
        
        # Step 6: Generate search index
        print("🔍 Building search index...")
        self.build_search_index()
        
        # Step 7: Generate navigation
        print("🧭 Building navigation structure...")
        self.generate_navigation()
        
        print("✅ Documentation generation complete!")

    def parse_headers(self):
        """Parse all header files in the include directory"""
        header_files = list(self.include_path.rglob("*.hpp"))
        
        for header_file in header_files:
            try:
                module = self.parse_header_file(header_file)
                self.modules[module.name] = module
                print(f"  ✓ Parsed {header_file.name}")
            except Exception as e:
                print(f"  ❌ Failed to parse {header_file.name}: {e}")

    def parse_header_file(self, header_path: Path) -> APIModule:
        """Parse a single header file"""
        with open(header_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        module_name = header_path.stem
        module = APIModule(
            name=module_name,
            file_path=str(header_path.relative_to(self.project_root)),
            description=self.extract_file_description(content)
        )
        
        # Extract includes
        module.includes = self.extract_includes(content)
        
        # Extract classes
        module.classes = self.extract_classes(content, str(header_path))
        
        # Extract standalone functions
        module.functions = self.extract_functions(content, str(header_path))
        
        # Extract enums
        module.enums = self.extract_enums(content)
        
        # Extract constants
        module.constants = self.extract_constants(content)
        
        return module

    def extract_file_description(self, content: str) -> str:
        """Extract file-level documentation"""
        # Look for file-level comments at the top
        pattern = r'/\*\*\s*\n\s*\*\s*@file.*?\n(.*?)\*/'
        match = re.search(pattern, content, re.DOTALL)
        
        if match:
            description = match.group(1)
            # Clean up the comment format
            description = re.sub(r'^\s*\*\s?', '', description, flags=re.MULTILINE)
            return description.strip()
        
        return ""

    def extract_includes(self, content: str) -> List[str]:
        """Extract #include statements"""
        pattern = r'#include\s*[<"](.*?)[>"]'
        return re.findall(pattern, content)

    def extract_classes(self, content: str, file_path: str) -> List[Class]:
        """Extract class and struct definitions"""
        classes = []
        
        # Pattern for class/struct with optional template
        class_pattern = r'(?:template\s*<([^>]*)>\s*)?(class|struct)\s+(\w+)(?:\s*:\s*([^{]+))?\s*{'
        
        for match in re.finditer(class_pattern, content):
            template_params_str, class_type, class_name, base_classes_str = match.groups()
            
            # Parse template parameters
            template_params = []
            if template_params_str:
                template_params = [param.strip() for param in template_params_str.split(',')]
            
            # Parse base classes
            base_classes = []
            if base_classes_str:
                base_classes = [base.strip() for base in base_classes_str.split(',')]
            
            class_obj = Class(
                name=class_name,
                namespace=self.extract_namespace_at_position(content, match.start()),
                template_params=template_params,
                base_classes=base_classes,
                file_location=file_path,
                line_number=content[:match.start()].count('\n') + 1,
                is_template=bool(template_params_str)
            )
            
            # Extract class documentation
            class_obj.doc = self.extract_doc_comment_before(content, match.start())
            
            # Extract methods and members
            class_body = self.extract_class_body(content, match.end() - 1)
            if class_body:
                class_obj.methods = self.extract_methods(class_body, class_name, file_path)
                class_obj.members = self.extract_members(class_body)
            
            classes.append(class_obj)
        
        return classes

    def extract_class_body(self, content: str, start_pos: int) -> Optional[str]:
        """Extract the body of a class (between braces)"""
        brace_count = 0
        body_start = start_pos
        
        for i, char in enumerate(content[start_pos:], start_pos):
            if char == '{':
                if brace_count == 0:
                    body_start = i + 1
                brace_count += 1
            elif char == '}':
                brace_count -= 1
                if brace_count == 0:
                    return content[body_start:i]
        
        return None

    def extract_methods(self, class_body: str, class_name: str, file_path: str) -> List[Function]:
        """Extract methods from class body"""
        methods = []
        
        # Pattern for methods (simplified)
        method_pattern = r'(?:(virtual|static|inline)\s+)?' \
                        r'(?:(const\s+)?)?' \
                        r'(\w+(?:\s*<[^>]*>)?(?:\s*\*|\s*&)?)\s+' \
                        r'(\w+)\s*\(([^)]*)\)' \
                        r'(?:\s*(const))?' \
                        r'(?:\s*(override|final))?' \
                        r'(?:\s*=\s*0)?' \
                        r'\s*[;{]'
        
        for match in re.finditer(method_pattern, class_body):
            modifier, const_prefix, return_type, method_name, params_str, const_suffix, override_final = match.groups()
            
            # Skip constructors, destructors, operators for now (simplified)
            if method_name == class_name or method_name.startswith('~') or method_name.startswith('operator'):
                continue
            
            method = Function(
                name=method_name,
                signature=match.group(0).strip().rstrip(';{'),
                namespace="",  # Will be filled by class namespace
                class_name=class_name,
                return_type=return_type.strip(),
                is_const=bool(const_suffix or const_prefix),
                is_static=bool(modifier and 'static' in modifier),
                is_virtual=bool(modifier and 'virtual' in modifier),
                file_location=file_path
            )
            
            # Parse parameters
            if params_str.strip():
                method.parameters = self.parse_parameters(params_str)
            
            # Extract method documentation
            method.doc = self.extract_doc_comment_before(class_body, match.start())
            
            methods.append(method)
        
        return methods

    def extract_functions(self, content: str, file_path: str) -> List[Function]:
        """Extract standalone functions"""
        functions = []
        
        # Simple function pattern (not inside classes)
        function_pattern = r'(?:inline\s+)?(?:static\s+)?' \
                          r'(\w+(?:\s*<[^>]*>)?(?:\s*\*|\s*&)?)\s+' \
                          r'(\w+)\s*\(([^)]*)\)' \
                          r'(?:\s*const)?' \
                          r'\s*[;{]'
        
        for match in re.finditer(function_pattern, content):
            return_type, func_name, params_str = match.groups()
            
            # Skip if inside a class (rough check)
            if self.is_inside_class(content, match.start()):
                continue
            
            func = Function(
                name=func_name,
                signature=match.group(0).strip().rstrip(';{'),
                namespace=self.extract_namespace_at_position(content, match.start()),
                return_type=return_type.strip(),
                file_location=file_path,
                line_number=content[:match.start()].count('\n') + 1
            )
            
            # Parse parameters
            if params_str.strip():
                func.parameters = self.parse_parameters(params_str)
            
            # Extract function documentation
            func.doc = self.extract_doc_comment_before(content, match.start())
            
            functions.append(func)
        
        return functions

    def extract_doc_comment_before(self, content: str, position: int) -> DocComment:
        """Extract documentation comment before a position"""
        # Look backwards for /** comment or /// comments
        lines = content[:position].split('\n')
        doc_lines = []
        
        # Collect doc comment lines working backwards
        for line in reversed(lines):
            line = line.strip()
            if line.startswith('*') or line.startswith('///'):
                doc_lines.insert(0, line)
            elif line == '/**' or line.startswith('/**'):
                doc_lines.insert(0, line)
                break
            elif line and not line.startswith('//'):
                break
        
        if not doc_lines:
            return DocComment()
        
        # Parse the documentation
        return self.parse_doc_comment('\n'.join(doc_lines))

    def parse_doc_comment(self, comment: str) -> DocComment:
        """Parse a documentation comment into structured data"""
        doc = DocComment()
        
        # Clean up comment markers
        cleaned = re.sub(r'^\s*/?(\*+|///)\s?', '', comment, flags=re.MULTILINE)
        cleaned = cleaned.replace('*/', '').strip()
        
        if not cleaned:
            return doc
        
        # Split into sections
        sections = {}
        current_section = 'description'
        current_content = []
        
        for line in cleaned.split('\n'):
            line = line.strip()
            
            # Check for special tags
            if line.startswith('@brief'):
                doc.brief = line[6:].strip()
                continue
            elif line.startswith('@param'):
                match = re.match(r'@param\s+(\w+)\s+(.*)', line)
                if match:
                    param_name, param_desc = match.groups()
                    doc.params[param_name] = param_desc
                continue
            elif line.startswith('@return'):
                doc.returns = line[7:].strip()
                continue
            elif line.startswith('@see'):
                doc.see_also.append(line[4:].strip())
                continue
            elif line.startswith('@since'):
                doc.since = line[6:].strip()
                continue
            elif line.startswith('@deprecated'):
                doc.deprecated = line[11:].strip()
                continue
            elif line.startswith('@note'):
                doc.notes.append(line[5:].strip())
                continue
            elif line.startswith('@example'):
                current_section = 'example'
                current_content = []
                continue
            
            if current_section == 'example':
                if line.startswith('@'):
                    # End of example
                    if current_content:
                        doc.examples.append('\n'.join(current_content))
                    current_section = 'description'
                    current_content = []
                else:
                    current_content.append(line)
            else:
                if not doc.description and not doc.brief:
                    doc.description += line + '\n'
        
        # Handle remaining example content
        if current_section == 'example' and current_content:
            doc.examples.append('\n'.join(current_content))
        
        doc.description = doc.description.strip()
        return doc

    def parse_parameters(self, params_str: str) -> List[Tuple[str, str]]:
        """Parse parameter list"""
        if not params_str.strip():
            return []
        
        params = []
        for param in params_str.split(','):
            param = param.strip()
            if not param:
                continue
            
            # Split type and name (simplified)
            parts = param.split()
            if len(parts) >= 2:
                param_type = ' '.join(parts[:-1])
                param_name = parts[-1].strip('&*')
                params.append((param_type, param_name))
            else:
                params.append((param, ''))
        
        return params

    def extract_namespace_at_position(self, content: str, position: int) -> str:
        """Extract the namespace at a given position"""
        # Look for namespace declarations before this position
        namespace_pattern = r'namespace\s+(\w+)\s*{'
        namespaces = []
        
        for match in re.finditer(namespace_pattern, content[:position]):
            namespaces.append(match.group(1))
        
        return '::'.join(namespaces) if namespaces else ""

    def is_inside_class(self, content: str, position: int) -> bool:
        """Check if position is inside a class definition"""
        # Count class/struct declarations vs closing braces before position
        class_count = len(re.findall(r'\b(class|struct)\s+\w+[^;]*{', content[:position]))
        brace_close_count = content[:position].count('}')
        
        # Rough approximation
        return class_count > brace_close_count

    def extract_enums(self, content: str) -> List[Dict]:
        """Extract enum definitions"""
        enums = []
        enum_pattern = r'enum\s+(?:class\s+)?(\w+)\s*(?::\s*(\w+))?\s*{([^}]*)}'
        
        for match in re.finditer(enum_pattern, content):
            enum_name, enum_type, enum_body = match.groups()
            
            values = []
            for value_match in re.finditer(r'(\w+)(?:\s*=\s*([^,}]+))?,?', enum_body):
                value_name, value_expr = value_match.groups()
                values.append({
                    'name': value_name.strip(),
                    'value': value_expr.strip() if value_expr else None
                })
            
            enums.append({
                'name': enum_name,
                'type': enum_type or 'int',
                'values': values,
                'line_number': content[:match.start()].count('\n') + 1
            })
        
        return enums

    def extract_constants(self, content: str) -> List[Dict]:
        """Extract constant definitions"""
        constants = []
        
        # constexpr and const static members
        const_pattern = r'(?:static\s+)?(?:constexpr|const)\s+(\w+(?:\s*<[^>]*>)?)\s+(\w+)\s*=\s*([^;]+);'
        
        for match in re.finditer(const_pattern, content):
            const_type, const_name, const_value = match.groups()
            
            constants.append({
                'name': const_name.strip(),
                'type': const_type.strip(),
                'value': const_value.strip(),
                'line_number': content[:match.start()].count('\n') + 1
            })
        
        return constants

    def extract_examples(self):
        """Extract code examples from example files"""
        if not self.examples_path.exists():
            return
        
        example_files = list(self.examples_path.rglob("*.cpp"))
        
        for example_file in example_files:
            try:
                examples = self.parse_example_file(example_file)
                
                # Associate examples with relevant modules
                for example in examples:
                    self.associate_example_with_modules(example)
                
                print(f"  ✓ Extracted examples from {example_file.name}")
            except Exception as e:
                print(f"  ❌ Failed to extract examples from {example_file.name}: {e}")

    def parse_example_file(self, example_path: Path) -> List[Dict]:
        """Parse an example file and extract documented examples"""
        with open(example_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        examples = []
        
        # Look for example sections marked with special comments
        example_pattern = r'/\*\*\s*@example\s+(.*?)\n(.*?)\*/(.*?)(?=/\*\*|\Z)'
        
        for match in re.finditer(example_pattern, content, re.DOTALL):
            title, description, code = match.groups()
            
            examples.append({
                'title': title.strip(),
                'description': description.strip(),
                'code': code.strip(),
                'file': str(example_path.relative_to(self.project_root)),
                'language': 'cpp'
            })
        
        # If no explicit examples found, treat whole file as example
        if not examples:
            examples.append({
                'title': example_path.stem.replace('_', ' ').title(),
                'description': f"Complete example from {example_path.name}",
                'code': content,
                'file': str(example_path.relative_to(self.project_root)),
                'language': 'cpp'
            })
        
        return examples

    def associate_example_with_modules(self, example: Dict):
        """Associate an example with relevant modules based on includes"""
        code = example['code']
        
        # Find includes in the example
        includes = re.findall(r'#include\s*[<"]ecscope/([^/>]*)[>"]', code)
        
        for include in includes:
            module_name = include.replace('.hpp', '')
            if module_name in self.modules:
                self.modules[module_name].examples.append(example)

    def generate_api_docs(self):
        """Generate API documentation files"""
        api_dir = self.docs_path / "api"
        api_dir.mkdir(exist_ok=True)
        
        # Generate module documentation
        for module_name, module in self.modules.items():
            self.generate_module_doc(module, api_dir)
        
        # Generate API overview
        self.generate_api_overview(api_dir)
        
        # Generate cross-references
        self.generate_cross_references(api_dir)

    def generate_module_doc(self, module: APIModule, output_dir: Path):
        """Generate documentation for a single module"""
        doc_content = self.render_module_template(module)
        
        output_file = output_dir / f"{module.name}.html"
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(doc_content)

    def render_module_template(self, module: APIModule) -> str:
        """Render module documentation using template"""
        return f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{module.name} - ECScope API</title>
    <link rel="stylesheet" href="../styles/main.css">
    <link rel="stylesheet" href="../styles/api.css">
</head>
<body>
    <div class="api-documentation">
        <header class="api-header">
            <h1>{module.name}</h1>
            <p class="module-path">{module.file_path}</p>
            {f'<p class="module-description">{module.description}</p>' if module.description else ''}
        </header>
        
        <nav class="api-nav">
            <ul>
                {f'<li><a href="#classes">Classes ({len(module.classes)})</a></li>' if module.classes else ''}
                {f'<li><a href="#functions">Functions ({len(module.functions)})</a></li>' if module.functions else ''}
                {f'<li><a href="#enums">Enums ({len(module.enums)})</a></li>' if module.enums else ''}
                {f'<li><a href="#examples">Examples ({len(module.examples)})</a></li>' if module.examples else ''}
            </ul>
        </nav>
        
        <main class="api-content">
            {self.render_classes_section(module.classes) if module.classes else ''}
            {self.render_functions_section(module.functions) if module.functions else ''}
            {self.render_enums_section(module.enums) if module.enums else ''}
            {self.render_examples_section(module.examples) if module.examples else ''}
        </main>
    </div>
    
    <script src="../scripts/api.js"></script>
</body>
</html>"""

    def render_classes_section(self, classes: List[Class]) -> str:
        """Render classes section"""
        if not classes:
            return ""
        
        section = '<section id="classes"><h2>Classes</h2>'
        
        for cls in classes:
            section += f'''
            <div class="class-doc" id="{cls.name}">
                <div class="class-header">
                    <h3>{cls.name}</h3>
                    {f'<div class="template-params">template&lt;{", ".join(cls.template_params)}&gt;</div>' if cls.template_params else ''}
                    {f'<div class="inheritance">: {", ".join(cls.base_classes)}</div>' if cls.base_classes else ''}
                </div>
                
                {f'<p class="brief">{cls.doc.brief}</p>' if cls.doc.brief else ''}
                {f'<div class="description">{cls.doc.description.replace(chr(10), "<br>")}</div>' if cls.doc.description else ''}
                
                {self.render_methods_subsection(cls.methods) if cls.methods else ''}
            </div>
            '''
        
        section += '</section>'
        return section

    def render_methods_subsection(self, methods: List[Function]) -> str:
        """Render methods subsection"""
        if not methods:
            return ""
        
        subsection = '<div class="methods"><h4>Methods</h4>'
        
        for method in methods:
            subsection += f'''
            <div class="method-doc">
                <div class="method-signature">
                    <code>{method.signature}</code>
                    {f'<span class="method-modifiers">{self.get_method_modifiers(method)}</span>' if self.get_method_modifiers(method) else ''}
                </div>
                {f'<p class="brief">{method.doc.brief}</p>' if method.doc.brief else ''}
                {f'<div class="description">{method.doc.description.replace(chr(10), "<br>")}</div>' if method.doc.description else ''}
                {self.render_parameters(method.parameters, method.doc.params) if method.parameters else ''}
                {f'<div class="returns"><strong>Returns:</strong> {method.doc.returns}</div>' if method.doc.returns else ''}
            </div>
            '''
        
        subsection += '</div>'
        return subsection

    def get_method_modifiers(self, method: Function) -> str:
        """Get method modifier string"""
        modifiers = []
        if method.is_static:
            modifiers.append('static')
        if method.is_virtual:
            modifiers.append('virtual')
        if method.is_const:
            modifiers.append('const')
        return ' '.join(modifiers)

    def render_parameters(self, params: List[Tuple[str, str]], param_docs: Dict[str, str]) -> str:
        """Render parameters section"""
        if not params:
            return ""
        
        section = '<div class="parameters"><strong>Parameters:</strong><ul>'
        
        for param_type, param_name in params:
            doc = param_docs.get(param_name, '')
            section += f'<li><code>{param_type} {param_name}</code>'
            if doc:
                section += f' - {doc}'
            section += '</li>'
        
        section += '</ul></div>'
        return section

    def render_functions_section(self, functions: List[Function]) -> str:
        """Render functions section"""
        if not functions:
            return ""
        
        section = '<section id="functions"><h2>Functions</h2>'
        
        for func in functions:
            section += f'''
            <div class="function-doc" id="{func.name}">
                <div class="function-signature">
                    <code>{func.signature}</code>
                </div>
                {f'<p class="brief">{func.doc.brief}</p>' if func.doc.brief else ''}
                {f'<div class="description">{func.doc.description.replace(chr(10), "<br>")}</div>' if func.doc.description else ''}
                {self.render_parameters(func.parameters, func.doc.params) if func.parameters else ''}
                {f'<div class="returns"><strong>Returns:</strong> {func.doc.returns}</div>' if func.doc.returns else ''}
            </div>
            '''
        
        section += '</section>'
        return section

    def render_enums_section(self, enums: List[Dict]) -> str:
        """Render enums section"""
        if not enums:
            return ""
        
        section = '<section id="enums"><h2>Enumerations</h2>'
        
        for enum in enums:
            section += f'''
            <div class="enum-doc" id="{enum['name']}">
                <h3>{enum['name']}</h3>
                <div class="enum-values">
                    <ul>
            '''
            
            for value in enum['values']:
                section += f'<li><code>{value["name"]}</code>'
                if value['value']:
                    section += f' = <code>{value["value"]}</code>'
                section += '</li>'
            
            section += '''
                    </ul>
                </div>
            </div>
            '''
        
        section += '</section>'
        return section

    def render_examples_section(self, examples: List[Dict]) -> str:
        """Render examples section"""
        if not examples:
            return ""
        
        section = '<section id="examples"><h2>Examples</h2>'
        
        for i, example in enumerate(examples):
            section += f'''
            <div class="example-doc" id="example-{i}">
                <h3>{example['title']}</h3>
                {f'<p class="example-description">{example["description"]}</p>' if example['description'] else ''}
                <div class="code-example">
                    <div class="code-header">
                        <span class="code-lang">{example['language']}</span>
                        <button class="copy-button" data-copy="{self.escape_for_html(example['code'])}">
                            <i class="fas fa-copy"></i>
                        </button>
                    </div>
                    <pre><code class="language-{example['language']}">{self.escape_for_html(example['code'])}</code></pre>
                </div>
            </div>
            '''
        
        section += '</section>'
        return section

    def escape_for_html(self, text: str) -> str:
        """Escape text for HTML"""
        return text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;')

    def generate_api_overview(self, api_dir: Path):
        """Generate API overview page"""
        overview_content = self.render_api_overview_template()
        
        with open(api_dir / "index.html", 'w', encoding='utf-8') as f:
            f.write(overview_content)

    def render_api_overview_template(self) -> str:
        """Render API overview template"""
        return f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>API Overview - ECScope</title>
    <link rel="stylesheet" href="../styles/main.css">
    <link rel="stylesheet" href="../styles/api.css">
</head>
<body>
    <div class="api-overview">
        <header>
            <h1>ECScope API Reference</h1>
            <p class="subtitle">Complete API documentation for ECScope Educational ECS Engine</p>
        </header>
        
        <main>
            <section class="modules-grid">
                {self.render_modules_grid()}
            </section>
            
            <section class="quick-reference">
                <h2>Quick Reference</h2>
                {self.render_quick_reference()}
            </section>
        </main>
    </div>
    
    <script src="../scripts/api.js"></script>
</body>
</html>"""

    def render_modules_grid(self) -> str:
        """Render modules grid"""
        grid = ""
        
        for module_name, module in self.modules.items():
            class_count = len(module.classes)
            function_count = len(module.functions)
            
            grid += f'''
            <div class="module-card">
                <h3><a href="{module_name}.html">{module_name}</a></h3>
                <p class="module-summary">{module.description[:100]}...</p>
                <div class="module-stats">
                    <span>{class_count} classes</span>
                    <span>{function_count} functions</span>
                </div>
            </div>
            '''
        
        return grid

    def render_quick_reference(self) -> str:
        """Render quick reference section"""
        # Get most commonly used classes/functions
        return '''
        <div class="quick-ref-grid">
            <div class="ref-column">
                <h3>Core Types</h3>
                <ul>
                    <li><a href="core.html#EntityID">EntityID</a></li>
                    <li><a href="ecs.html#Registry">Registry</a></li>
                    <li><a href="ecs.html#Component">Component</a></li>
                    <li><a href="ecs.html#System">System</a></li>
                </ul>
            </div>
            
            <div class="ref-column">
                <h3>Memory Management</h3>
                <ul>
                    <li><a href="memory.html#Arena">Arena</a></li>
                    <li><a href="memory.html#Pool">Pool</a></li>
                    <li><a href="memory.html#MemoryTracker">MemoryTracker</a></li>
                </ul>
            </div>
            
            <div class="ref-column">
                <h3>Performance</h3>
                <ul>
                    <li><a href="performance.html#Profiler">Profiler</a></li>
                    <li><a href="performance.html#Benchmarker">Benchmarker</a></li>
                </ul>
            </div>
        </div>
        '''

    def generate_cross_references(self, api_dir: Path):
        """Generate cross-reference data"""
        cross_refs = {
            'classes': {},
            'functions': {},
            'dependencies': {},
            'inheritance': {}
        }
        
        # Build cross-reference data
        for module_name, module in self.modules.items():
            for cls in module.classes:
                cross_refs['classes'][f"{module_name}::{cls.name}"] = {
                    'module': module_name,
                    'file': module.file_path,
                    'methods': [m.name for m in cls.methods],
                    'base_classes': cls.base_classes
                }
                
                # Track inheritance
                for base in cls.base_classes:
                    if base not in cross_refs['inheritance']:
                        cross_refs['inheritance'][base] = []
                    cross_refs['inheritance'][base].append(f"{module_name}::{cls.name}")
        
        # Save cross-reference data
        with open(api_dir / "cross_refs.json", 'w', encoding='utf-8') as f:
            json.dump(cross_refs, f, indent=2)

    def generate_interactive_tutorials(self):
        """Generate interactive tutorial content"""
        tutorials_dir = self.interactive_path / "tutorials"
        tutorials_dir.mkdir(exist_ok=True)
        
        # Generate tutorial definitions
        self.generate_tutorial_definitions(tutorials_dir)
        
        # Generate interactive code examples
        self.generate_interactive_examples(tutorials_dir)

    def generate_tutorial_definitions(self, tutorials_dir: Path):
        """Generate tutorial definition files"""
        tutorials = {
            'basic-ecs': self.generate_basic_ecs_tutorial(),
            'memory-lab': self.generate_memory_lab_tutorial(),
            'physics': self.generate_physics_tutorial(),
            'rendering': self.generate_rendering_tutorial(),
            'advanced': self.generate_advanced_tutorial()
        }
        
        for tutorial_id, content in tutorials.items():
            with open(tutorials_dir / f"{tutorial_id}.html", 'w', encoding='utf-8') as f:
                f.write(content)

    def generate_basic_ecs_tutorial(self) -> str:
        """Generate basic ECS tutorial content"""
        return '''
        <div class="tutorial-content">
            <div class="tutorial-step" data-step="1">
                <h2>Understanding Entities</h2>
                <p>In ECScope, entities are lightweight identifiers that tie components together.</p>
                
                <div class="interactive-demo" id="entity-demo">
                    <div class="demo-controls">
                        <button onclick="createEntity()">Create Entity</button>
                        <button onclick="destroyEntity()">Destroy Entity</button>
                    </div>
                    <div class="entity-visualization"></div>
                </div>
                
                <div class="code-example">
                    <pre><code class="language-cpp">
// Create an entity
auto entity = registry.create_entity();
std::cout << "Created entity: " << entity << std::endl;

// Entities are just IDs - lightweight and fast
static_assert(sizeof(EntityID) == sizeof(uint32_t));
                    </code></pre>
                </div>
            </div>
        </div>
        '''

    def generate_performance_docs(self):
        """Generate performance documentation"""
        perf_dir = self.docs_path / "performance"
        perf_dir.mkdir(exist_ok=True)
        
        # Generate benchmark documentation
        self.generate_benchmark_docs(perf_dir)
        
        # Generate optimization guides
        self.generate_optimization_guides(perf_dir)

    def build_search_index(self):
        """Build search index for documentation"""
        search_data = {
            'documents': [],
            'index': {}
        }
        
        # Index all API documentation
        for module_name, module in self.modules.items():
            # Index module
            search_data['documents'].append({
                'id': f"module_{module_name}",
                'title': module_name,
                'type': 'module',
                'content': module.description,
                'url': f"api/{module_name}.html"
            })
            
            # Index classes
            for cls in module.classes:
                search_data['documents'].append({
                    'id': f"class_{module_name}_{cls.name}",
                    'title': f"{cls.name} (class)",
                    'type': 'class',
                    'content': f"{cls.doc.brief} {cls.doc.description}",
                    'url': f"api/{module_name}.html#{cls.name}"
                })
            
            # Index functions
            for func in module.functions:
                search_data['documents'].append({
                    'id': f"function_{module_name}_{func.name}",
                    'title': f"{func.name}() (function)",
                    'type': 'function',
                    'content': f"{func.doc.brief} {func.doc.description}",
                    'url': f"api/{module_name}.html#{func.name}"
                })
        
        # Save search index
        with open(self.interactive_path / "data" / "search_index.json", 'w', encoding='utf-8') as f:
            json.dump(search_data, f, indent=2)

    def generate_navigation(self):
        """Generate navigation structure"""
        nav_data = {
            'sections': [
                {
                    'title': 'Getting Started',
                    'items': [
                        {'title': 'Quick Start', 'url': '#quick-start'},
                        {'title': 'Installation', 'url': '#installation'},
                        {'title': 'First Application', 'url': '#first-app'}
                    ]
                },
                {
                    'title': 'API Reference',
                    'items': [
                        {'title': module.name.title(), 'url': f'api/{module.name}.html'}
                        for module in self.modules.values()
                    ]
                },
                {
                    'title': 'Tutorials',
                    'items': [
                        {'title': 'Basic ECS', 'url': '#tutorial-basic-ecs'},
                        {'title': 'Memory Lab', 'url': '#tutorial-memory-lab'},
                        {'title': 'Physics System', 'url': '#tutorial-physics'},
                        {'title': 'Rendering', 'url': '#tutorial-rendering'}
                    ]
                }
            ]
        }
        
        # Ensure data directory exists
        data_dir = self.interactive_path / "data"
        data_dir.mkdir(exist_ok=True)
        
        with open(data_dir / "navigation.json", 'w', encoding='utf-8') as f:
            json.dump(nav_data, f, indent=2)

def main():
    parser = argparse.ArgumentParser(description='Generate ECScope documentation')
    parser.add_argument('project_root', help='Path to ECScope project root')
    parser.add_argument('--output', help='Output directory for documentation')
    parser.add_argument('--api-only', action='store_true', help='Generate only API documentation')
    parser.add_argument('--interactive-only', action='store_true', help='Generate only interactive content')
    
    args = parser.parse_args()
    
    project_root = Path(args.project_root).resolve()
    if not project_root.exists():
        print(f"❌ Project root '{project_root}' does not exist")
        return 1
    
    generator = ECScope_DocGenerator(str(project_root))
    
    try:
        if args.api_only:
            generator.parse_headers()
            generator.extract_examples()
            generator.generate_api_docs()
        elif args.interactive_only:
            generator.generate_interactive_tutorials()
            generator.generate_performance_docs()
        else:
            generator.generate_all_documentation()
        
        print("✅ Documentation generation completed successfully!")
        return 0
        
    except Exception as e:
        print(f"❌ Documentation generation failed: {e}")
        import traceback
        traceback.print_exc()
        return 1

if __name__ == '__main__':
    exit(main())