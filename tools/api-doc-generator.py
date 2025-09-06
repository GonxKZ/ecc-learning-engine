#!/usr/bin/env python3
"""
ECScope API Documentation Generator

This script automatically generates comprehensive API documentation from ECScope
source code, including interactive examples, performance characteristics,
and educational annotations.

Features:
- C++ header parsing using libclang
- Interactive code example generation
- Performance benchmark integration
- Educational content generation
- Multi-format output (JSON, HTML, Markdown)
- Cross-referencing with examples and tutorials

Usage:
    python api-doc-generator.py --source include/ --output docs/interactive/generated/
"""

import os
import sys
import json
import re
import argparse
import logging
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple, Any
from dataclasses import dataclass, asdict
from datetime import datetime

try:
    import clang.cindex
    from clang.cindex import Index, CursorKind, TypeKind
except ImportError:
    print("Error: python-clang not found. Install with: pip install libclang")
    sys.exit(1)

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

@dataclass
class Parameter:
    """Function/method parameter information"""
    name: str
    type: str
    description: str = ""
    default_value: Optional[str] = None
    is_const: bool = False
    is_reference: bool = False
    is_pointer: bool = False

@dataclass
class Function:
    """Function/method documentation"""
    name: str
    return_type: str
    parameters: List[Parameter]
    description: str = ""
    brief: str = ""
    example_code: str = ""
    performance_notes: str = ""
    complexity: str = ""
    is_const: bool = False
    is_static: bool = False
    is_virtual: bool = False
    is_template: bool = False
    template_parameters: List[str] = None
    line_number: int = 0
    file_path: str = ""

@dataclass
class Class:
    """Class documentation"""
    name: str
    description: str = ""
    brief: str = ""
    base_classes: List[str] = None
    template_parameters: List[str] = None
    public_methods: List[Function] = None
    private_methods: List[Function] = None
    protected_methods: List[Function] = None
    public_members: List[Parameter] = None
    private_members: List[Parameter] = None
    protected_members: List[Parameter] = None
    nested_classes: List['Class'] = None
    example_usage: str = ""
    performance_characteristics: Dict[str, Any] = None
    educational_notes: List[str] = None
    line_number: int = 0
    file_path: str = ""

@dataclass
class Namespace:
    """Namespace documentation"""
    name: str
    description: str = ""
    classes: List[Class] = None
    functions: List[Function] = None
    nested_namespaces: List['Namespace'] = None

@dataclass
class APIDocumentation:
    """Complete API documentation"""
    project_name: str
    version: str
    namespaces: List[Namespace]
    global_functions: List[Function]
    generated_at: str
    source_files: List[str]
    
class ECSOcopeAPIParser:
    """Parse ECScope C++ headers and generate comprehensive documentation"""
    
    def __init__(self, source_dirs: List[str], examples_dir: str = None):
        self.source_dirs = [Path(d) for d in source_dirs]
        self.examples_dir = Path(examples_dir) if examples_dir else None
        
        # Initialize libclang
        clang.cindex.conf.set_library_file('/usr/lib/x86_64-linux-gnu/libclang-10.so.1')
        self.index = Index.create()
        
        # Compilation database
        self.compile_flags = [
            '-std=c++20',
            '-I./include',
            '-I./external',
            '-DECSCOPE_DOCUMENTATION_BUILD',
            '-x', 'c++'
        ]
        
        # Documentation storage
        self.namespaces: Dict[str, Namespace] = {}
        self.classes: Dict[str, Class] = {}
        self.functions: List[Function] = []
        
        # Code examples mapping
        self.examples_map: Dict[str, List[str]] = {}
        
        # Performance data integration
        self.performance_data: Dict[str, Dict] = {}
        
        # Educational content
        self.educational_content: Dict[str, List[str]] = {}
        
    def parse_all_headers(self) -> APIDocumentation:
        """Parse all header files and generate documentation"""
        logger.info("Starting API documentation generation...")
        
        # Find all header files
        header_files = self._find_header_files()
        logger.info(f"Found {len(header_files)} header files")
        
        # Load examples mapping
        if self.examples_dir:
            self._load_examples_mapping()
        
        # Load performance data
        self._load_performance_data()
        
        # Load educational content
        self._load_educational_content()
        
        # Parse each header file
        for header_file in header_files:
            logger.info(f"Parsing {header_file}")
            self._parse_header_file(header_file)
        
        # Build complete documentation
        api_doc = APIDocumentation(
            project_name="ECScope",
            version="1.0.0",
            namespaces=list(self.namespaces.values()),
            global_functions=self.functions,
            generated_at=datetime.now().isoformat(),
            source_files=[str(f) for f in header_files]
        )
        
        logger.info("API documentation generation completed")
        return api_doc
    
    def _find_header_files(self) -> List[Path]:
        """Find all C++ header files in source directories"""
        header_files = []
        
        for source_dir in self.source_dirs:
            if not source_dir.exists():
                logger.warning(f"Source directory does not exist: {source_dir}")
                continue
                
            # Find .hpp and .h files recursively
            for pattern in ['**/*.hpp', '**/*.h']:
                header_files.extend(source_dir.glob(pattern))
        
        return sorted(header_files)
    
    def _parse_header_file(self, header_file: Path):
        """Parse a single header file using libclang"""
        try:
            # Parse the file
            translation_unit = self.index.parse(
                str(header_file),
                args=self.compile_flags,
                options=clang.cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD
            )
            
            # Check for parse errors
            if translation_unit.diagnostics:
                for diag in translation_unit.diagnostics:
                    if diag.severity >= clang.cindex.Diagnostic.Warning:
                        logger.warning(f"Parse warning in {header_file}: {diag}")
            
            # Walk the AST and extract documentation
            self._walk_ast(translation_unit.cursor, str(header_file))
            
        except Exception as e:
            logger.error(f"Failed to parse {header_file}: {e}")
    
    def _walk_ast(self, cursor, file_path: str, namespace_stack: List[str] = None):
        """Walk the AST and extract documentation information"""
        if namespace_stack is None:
            namespace_stack = []
        
        # Only process cursors from the current file
        if cursor.location.file and str(cursor.location.file) != file_path:
            return
        
        cursor_kind = cursor.kind
        
        if cursor_kind == CursorKind.NAMESPACE:
            # Handle namespace
            namespace_name = cursor.spelling
            full_namespace = '::'.join(namespace_stack + [namespace_name])
            
            if full_namespace not in self.namespaces:
                self.namespaces[full_namespace] = Namespace(
                    name=full_namespace,
                    description=self._extract_documentation(cursor),
                    classes=[],
                    functions=[],
                    nested_namespaces=[]
                )
            
            # Recursively process namespace contents
            new_stack = namespace_stack + [namespace_name]
            for child in cursor.get_children():
                self._walk_ast(child, file_path, new_stack)
        
        elif cursor_kind == CursorKind.CLASS_DECL or cursor_kind == CursorKind.STRUCT_DECL:
            # Handle class/struct
            class_info = self._parse_class(cursor, file_path, namespace_stack)
            if class_info:
                full_name = '::'.join(namespace_stack + [class_info.name])
                self.classes[full_name] = class_info
                
                # Add to appropriate namespace
                namespace_name = '::'.join(namespace_stack) if namespace_stack else 'global'
                if namespace_name in self.namespaces:
                    self.namespaces[namespace_name].classes.append(class_info)
        
        elif cursor_kind == CursorKind.FUNCTION_DECL:
            # Handle free functions
            func_info = self._parse_function(cursor, file_path)
            if func_info:
                self.functions.append(func_info)
                
                # Add to appropriate namespace
                namespace_name = '::'.join(namespace_stack) if namespace_stack else 'global'
                if namespace_name in self.namespaces:
                    self.namespaces[namespace_name].functions.append(func_info)
        
        else:
            # Recursively process children
            for child in cursor.get_children():
                self._walk_ast(child, file_path, namespace_stack)
    
    def _parse_class(self, cursor, file_path: str, namespace_stack: List[str]) -> Optional[Class]:
        """Parse class/struct declaration"""
        if not cursor.spelling:
            return None
        
        class_info = Class(
            name=cursor.spelling,
            description=self._extract_documentation(cursor),
            brief=self._extract_brief(cursor),
            base_classes=[],
            template_parameters=[],
            public_methods=[],
            private_methods=[],
            protected_methods=[],
            public_members=[],
            private_members=[],
            protected_members=[],
            nested_classes=[],
            line_number=cursor.location.line,
            file_path=file_path
        )
        
        # Extract template parameters
        if cursor.kind == CursorKind.CLASS_TEMPLATE:
            class_info.template_parameters = self._extract_template_parameters(cursor)
        
        # Process class members
        current_access = 'public' if cursor.kind == CursorKind.STRUCT_DECL else 'private'
        
        for child in cursor.get_children():
            if child.kind == CursorKind.CXX_ACCESS_SPEC_DECL:
                access_specifier = child.access_specifier
                if access_specifier == clang.cindex.AccessSpecifier.PUBLIC:
                    current_access = 'public'
                elif access_specifier == clang.cindex.AccessSpecifier.PRIVATE:
                    current_access = 'private'
                elif access_specifier == clang.cindex.AccessSpecifier.PROTECTED:
                    current_access = 'protected'
                    
            elif child.kind == CursorKind.CXX_METHOD or child.kind == CursorKind.FUNCTION_DECL:
                method = self._parse_function(child, file_path)
                if method:
                    if current_access == 'public':
                        class_info.public_methods.append(method)
                    elif current_access == 'private':
                        class_info.private_methods.append(method)
                    else:
                        class_info.protected_methods.append(method)
                        
            elif child.kind == CursorKind.FIELD_DECL:
                member = self._parse_member_variable(child)
                if member:
                    if current_access == 'public':
                        class_info.public_members.append(member)
                    elif current_access == 'private':
                        class_info.private_members.append(member)
                    else:
                        class_info.protected_members.append(member)
                        
            elif child.kind == CursorKind.CXX_BASE_SPECIFIER:
                base_class = child.type.spelling
                class_info.base_classes.append(base_class)
        
        # Add examples and educational content
        class_name = '::'.join(namespace_stack + [class_info.name])
        class_info.example_usage = self._get_example_usage(class_name)
        class_info.performance_characteristics = self._get_performance_characteristics(class_name)
        class_info.educational_notes = self._get_educational_notes(class_name)
        
        return class_info
    
    def _parse_function(self, cursor, file_path: str) -> Optional[Function]:
        """Parse function/method declaration"""
        if not cursor.spelling:
            return None
        
        func_info = Function(
            name=cursor.spelling,
            return_type=cursor.result_type.spelling,
            parameters=[],
            description=self._extract_documentation(cursor),
            brief=self._extract_brief(cursor),
            is_const=cursor.is_const_method(),
            is_static=cursor.is_static_method(),
            is_virtual=cursor.is_virtual_method(),
            line_number=cursor.location.line,
            file_path=file_path
        )
        
        # Extract parameters
        for arg in cursor.get_arguments():
            param = Parameter(
                name=arg.spelling or f"arg{len(func_info.parameters)}",
                type=arg.type.spelling,
                is_const=arg.type.is_const_qualified(),
                is_reference=arg.type.kind == TypeKind.LVALUEREFERENCE,
                is_pointer=arg.type.kind == TypeKind.POINTER
            )
            func_info.parameters.append(param)
        
        # Extract template parameters
        if cursor.kind == CursorKind.FUNCTION_TEMPLATE:
            func_info.is_template = True
            func_info.template_parameters = self._extract_template_parameters(cursor)
        
        # Add performance and educational information
        func_name = func_info.name
        func_info.example_code = self._get_function_example(func_name)
        func_info.performance_notes = self._get_performance_notes(func_name)
        func_info.complexity = self._get_complexity_analysis(func_name)
        
        return func_info
    
    def _parse_member_variable(self, cursor) -> Optional[Parameter]:
        """Parse member variable declaration"""
        if not cursor.spelling:
            return None
        
        return Parameter(
            name=cursor.spelling,
            type=cursor.type.spelling,
            description=self._extract_documentation(cursor),
            is_const=cursor.type.is_const_qualified(),
            is_reference=cursor.type.kind == TypeKind.LVALUEREFERENCE,
            is_pointer=cursor.type.kind == TypeKind.POINTER
        )
    
    def _extract_documentation(self, cursor) -> str:
        """Extract documentation comment from cursor"""
        if cursor.raw_comment:
            # Parse Doxygen-style comments
            comment = cursor.raw_comment
            
            # Remove comment markers
            comment = re.sub(r'^\s*[/\*]+', '', comment, flags=re.MULTILINE)
            comment = re.sub(r'\*+/\s*$', '', comment, flags=re.MULTILINE)
            comment = re.sub(r'^\s*\*\s?', '', comment, flags=re.MULTILINE)
            
            # Extract main description (everything before @param, @return, etc.)
            description_match = re.search(r'^(.*?)(?=@\w+|$)', comment, re.DOTALL)
            if description_match:
                return description_match.group(1).strip()
        
        return ""
    
    def _extract_brief(self, cursor) -> str:
        """Extract brief description from documentation"""
        if cursor.raw_comment:
            # Look for @brief tag
            brief_match = re.search(r'@brief\s+(.*?)(?=\n\s*(?:@|\*\/|$))', cursor.raw_comment, re.DOTALL)
            if brief_match:
                return brief_match.group(1).strip()
            
            # Use first sentence as brief if no @brief tag
            doc = self._extract_documentation(cursor)
            if doc:
                first_sentence = re.search(r'^([^.!?]*[.!?])', doc)
                if first_sentence:
                    return first_sentence.group(1).strip()
        
        return ""
    
    def _extract_template_parameters(self, cursor) -> List[str]:
        """Extract template parameter names"""
        template_params = []
        for child in cursor.get_children():
            if child.kind == CursorKind.TEMPLATE_TYPE_PARAMETER:
                template_params.append(child.spelling or f"T{len(template_params)}")
            elif child.kind == CursorKind.TEMPLATE_NON_TYPE_PARAMETER:
                template_params.append(child.spelling or f"N{len(template_params)}")
        return template_params
    
    def _load_examples_mapping(self):
        """Load mapping between API elements and code examples"""
        if not self.examples_dir.exists():
            logger.warning(f"Examples directory does not exist: {self.examples_dir}")
            return
        
        # Scan example files and extract relationships
        for example_file in self.examples_dir.rglob("*.cpp"):
            with open(example_file, 'r') as f:
                content = f.read()
                
                # Look for class/function usage patterns
                api_elements = self._extract_api_usage(content)
                
                for element in api_elements:
                    if element not in self.examples_map:
                        self.examples_map[element] = []
                    self.examples_map[element].append(str(example_file))
    
    def _extract_api_usage(self, code: str) -> Set[str]:
        """Extract API element usage from code"""
        api_elements = set()
        
        # Look for class instantiations
        class_pattern = r'\b([A-Z][a-zA-Z0-9_]*)\s+\w+\s*[({]'
        for match in re.finditer(class_pattern, code):
            api_elements.add(match.group(1))
        
        # Look for method calls
        method_pattern = r'\.([a-zA-Z_][a-zA-Z0-9_]*)\s*\('
        for match in re.finditer(method_pattern, code):
            api_elements.add(match.group(1))
        
        # Look for function calls
        function_pattern = r'\b([a-z_][a-zA-Z0-9_]*)\s*\('
        for match in re.finditer(function_pattern, code):
            api_elements.add(match.group(1))
        
        return api_elements
    
    def _load_performance_data(self):
        """Load performance characteristics data"""
        perf_data_file = Path("benchmarks/performance_characteristics.json")
        if perf_data_file.exists():
            try:
                with open(perf_data_file) as f:
                    self.performance_data = json.load(f)
            except Exception as e:
                logger.warning(f"Failed to load performance data: {e}")
    
    def _load_educational_content(self):
        """Load educational content for API elements"""
        edu_content_file = Path("docs/educational_content.json")
        if edu_content_file.exists():
            try:
                with open(edu_content_file) as f:
                    self.educational_content = json.load(f)
            except Exception as e:
                logger.warning(f"Failed to load educational content: {e}")
    
    def _get_example_usage(self, class_name: str) -> str:
        """Get example usage for a class"""
        if class_name in self.examples_map:
            # Read first example file
            example_file = self.examples_map[class_name][0]
            try:
                with open(example_file) as f:
                    content = f.read()
                    
                    # Extract relevant code snippet
                    return self._extract_relevant_snippet(content, class_name)
            except:
                pass
        
        return ""
    
    def _extract_relevant_snippet(self, code: str, target: str) -> str:
        """Extract relevant code snippet for a target API element"""
        lines = code.split('\n')
        relevant_lines = []
        
        for i, line in enumerate(lines):
            if target in line:
                # Include some context
                start = max(0, i - 2)
                end = min(len(lines), i + 5)
                return '\n'.join(lines[start:end])
        
        return ""
    
    def _get_function_example(self, func_name: str) -> str:
        """Get example code for a function"""
        return self._get_example_usage(func_name)
    
    def _get_performance_characteristics(self, element_name: str) -> Dict[str, Any]:
        """Get performance characteristics for an API element"""
        return self.performance_data.get(element_name, {})
    
    def _get_performance_notes(self, func_name: str) -> str:
        """Get performance notes for a function"""
        perf_data = self.performance_data.get(func_name, {})
        return perf_data.get('notes', '')
    
    def _get_complexity_analysis(self, func_name: str) -> str:
        """Get complexity analysis for a function"""
        perf_data = self.performance_data.get(func_name, {})
        return perf_data.get('complexity', '')
    
    def _get_educational_notes(self, element_name: str) -> List[str]:
        """Get educational notes for an API element"""
        return self.educational_content.get(element_name, [])

class DocumentationExporter:
    """Export API documentation in various formats"""
    
    def __init__(self, api_doc: APIDocumentation):
        self.api_doc = api_doc
    
    def export_json(self, output_path: Path):
        """Export documentation as JSON"""
        logger.info(f"Exporting JSON documentation to {output_path}")
        
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        with open(output_path, 'w') as f:
            json.dump(asdict(self.api_doc), f, indent=2, default=str)
    
    def export_html(self, output_dir: Path):
        """Export documentation as interactive HTML"""
        logger.info(f"Exporting HTML documentation to {output_dir}")
        
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Generate main index
        self._generate_html_index(output_dir)
        
        # Generate namespace pages
        for namespace in self.api_doc.namespaces:
            self._generate_namespace_page(namespace, output_dir)
        
        # Generate class pages
        for namespace in self.api_doc.namespaces:
            for class_info in namespace.classes:
                self._generate_class_page(class_info, output_dir)
    
    def export_markdown(self, output_dir: Path):
        """Export documentation as Markdown files"""
        logger.info(f"Exporting Markdown documentation to {output_dir}")
        
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Generate README
        self._generate_markdown_readme(output_dir)
        
        # Generate namespace documentation
        for namespace in self.api_doc.namespaces:
            self._generate_namespace_markdown(namespace, output_dir)
    
    def _generate_html_index(self, output_dir: Path):
        """Generate HTML index page"""
        html_content = f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{self.api_doc.project_name} API Documentation</title>
    <link rel="stylesheet" href="styles/api-docs.css">
</head>
<body>
    <div class="container">
        <header>
            <h1>{self.api_doc.project_name} API Documentation</h1>
            <p>Generated on {self.api_doc.generated_at}</p>
        </header>
        
        <main>
            <section class="overview">
                <h2>Overview</h2>
                <p>Interactive API documentation for ECScope Educational ECS Engine.</p>
            </section>
            
            <section class="namespaces">
                <h2>Namespaces</h2>
                <ul class="namespace-list">
        """
        
        for namespace in self.api_doc.namespaces:
            html_content += f'''
                    <li>
                        <a href="namespace_{namespace.name.replace('::', '_')}.html">
                            <code>{namespace.name}</code>
                        </a>
                        <p>{namespace.description}</p>
                    </li>
            '''
        
        html_content += """
                </ul>
            </section>
        </main>
    </div>
    
    <script src="js/api-docs.js"></script>
</body>
</html>
        """
        
        with open(output_dir / "index.html", 'w') as f:
            f.write(html_content)
    
    def _generate_namespace_page(self, namespace: Namespace, output_dir: Path):
        """Generate HTML page for a namespace"""
        safe_name = namespace.name.replace('::', '_')
        filename = f"namespace_{safe_name}.html"
        
        html_content = f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Namespace {namespace.name} - {self.api_doc.project_name}</title>
    <link rel="stylesheet" href="styles/api-docs.css">
</head>
<body>
    <div class="container">
        <header>
            <h1>Namespace <code>{namespace.name}</code></h1>
            <p>{namespace.description}</p>
        </header>
        
        <main>
            <section class="classes">
                <h2>Classes</h2>
                <div class="class-grid">
        """
        
        for class_info in namespace.classes:
            html_content += f"""
                    <div class="class-card">
                        <h3><a href="class_{class_info.name}.html">{class_info.name}</a></h3>
                        <p>{class_info.brief}</p>
                    </div>
            """
        
        html_content += """
                </div>
            </section>
        </main>
    </div>
</body>
</html>
        """
        
        with open(output_dir / filename, 'w') as f:
            f.write(html_content)
    
    def _generate_class_page(self, class_info: Class, output_dir: Path):
        """Generate HTML page for a class"""
        filename = f"class_{class_info.name}.html"
        
        html_content = f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Class {class_info.name} - {self.api_doc.project_name}</title>
    <link rel="stylesheet" href="styles/api-docs.css">
    <script src="https://cdnjs.cloudflare.com/ajax/libs/prism/1.24.1/components/prism-core.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/prism/1.24.1/plugins/autoloader/prism-autoloader.min.js"></script>
</head>
<body>
    <div class="container">
        <header>
            <h1>Class <code>{class_info.name}</code></h1>
            <p>{class_info.description}</p>
        </header>
        
        <main>
            <section class="methods">
                <h2>Public Methods</h2>
        """
        
        for method in class_info.public_methods:
            html_content += self._generate_method_documentation(method)
        
        if class_info.example_usage:
            html_content += f"""
            </section>
            
            <section class="examples">
                <h2>Example Usage</h2>
                <pre><code class="language-cpp">{class_info.example_usage}</code></pre>
            </section>
            """
        
        html_content += """
        </main>
    </div>
</body>
</html>
        """
        
        with open(output_dir / filename, 'w') as f:
            f.write(html_content)
    
    def _generate_method_documentation(self, method: Function) -> str:
        """Generate HTML documentation for a method"""
        params_html = ""
        for param in method.parameters:
            params_html += f"<span class='param'>{param.type} {param.name}</span>, "
        params_html = params_html.rstrip(", ")
        
        return f"""
            <div class="method">
                <h3>{method.name}</h3>
                <div class="signature">
                    <code>{method.return_type} {method.name}({params_html})</code>
                </div>
                <p>{method.description}</p>
                
                {f'<div class="performance-notes"><strong>Performance:</strong> {method.performance_notes}</div>' if method.performance_notes else ''}
                {f'<div class="complexity"><strong>Complexity:</strong> {method.complexity}</div>' if method.complexity else ''}
                
                {f'<pre><code class="language-cpp">{method.example_code}</code></pre>' if method.example_code else ''}
            </div>
        """
    
    def _generate_markdown_readme(self, output_dir: Path):
        """Generate main README.md file"""
        content = f"""# {self.api_doc.project_name} API Reference

*Generated on {self.api_doc.generated_at}*

## Overview

This is the complete API reference for ECScope Educational ECS Engine.

## Namespaces

"""
        
        for namespace in self.api_doc.namespaces:
            content += f"- [`{namespace.name}`](namespace_{namespace.name.replace('::', '_')}.md) - {namespace.description}\n"
        
        with open(output_dir / "README.md", 'w') as f:
            f.write(content)
    
    def _generate_namespace_markdown(self, namespace: Namespace, output_dir: Path):
        """Generate Markdown documentation for a namespace"""
        safe_name = namespace.name.replace('::', '_')
        filename = f"namespace_{safe_name}.md"
        
        content = f"""# Namespace `{namespace.name}`

{namespace.description}

## Classes

"""
        
        for class_info in namespace.classes:
            content += f"""### `{class_info.name}`

{class_info.description}

#### Public Methods

"""
            for method in class_info.public_methods:
                content += f"""##### `{method.name}`

```cpp
{method.return_type} {method.name}(...)
```

{method.description}

"""
        
        with open(output_dir / filename, 'w') as f:
            f.write(content)

def main():
    parser = argparse.ArgumentParser(description="Generate ECScope API documentation")
    parser.add_argument('--source', required=True, nargs='+', help='Source directories to parse')
    parser.add_argument('--examples', help='Examples directory')
    parser.add_argument('--output', required=True, help='Output directory')
    parser.add_argument('--format', choices=['json', 'html', 'markdown', 'all'], default='all', help='Output format')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    
    args = parser.parse_args()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    # Parse API documentation
    parser = ECSOcopeAPIParser(args.source, args.examples)
    api_doc = parser.parse_all_headers()
    
    # Export documentation
    exporter = DocumentationExporter(api_doc)
    output_path = Path(args.output)
    
    if args.format in ['json', 'all']:
        exporter.export_json(output_path / 'api.json')
    
    if args.format in ['html', 'all']:
        exporter.export_html(output_path / 'html')
    
    if args.format in ['markdown', 'all']:
        exporter.export_markdown(output_path / 'markdown')
    
    logger.info("API documentation generation completed successfully!")

if __name__ == '__main__':
    main()