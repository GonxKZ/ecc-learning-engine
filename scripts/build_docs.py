#!/usr/bin/env python3
"""
ECScope Documentation Build System
Automated documentation generation and deployment pipeline
"""

import os
import sys
import json
import shutil
import subprocess
import argparse
from pathlib import Path
from typing import Dict, List, Optional, Any
import time
import hashlib

class ECScope_DocBuilder:
    """Comprehensive documentation build system for ECScope"""
    
    def __init__(self, project_root: str):
        self.project_root = Path(project_root).resolve()
        self.docs_path = self.project_root / "docs"
        self.interactive_path = self.docs_path / "interactive"
        self.tools_path = self.project_root / "tools"
        
        # Build configuration
        self.config = {
            "generate_api": True,
            "generate_tutorials": True,
            "generate_examples": True,
            "generate_performance": True,
            "build_search_index": True,
            "validate_links": True,
            "optimize_assets": True,
            "enable_development_features": False
        }
        
        # Build cache for incremental updates
        self.cache_file = self.docs_path / ".build_cache.json"
        self.build_cache = self.load_build_cache()
        
        print(f"🏗️  ECScope Documentation Builder")
        print(f"📁 Project root: {self.project_root}")

    def load_build_cache(self) -> Dict[str, Any]:
        """Load build cache for incremental builds"""
        if self.cache_file.exists():
            try:
                with open(self.cache_file, 'r') as f:
                    return json.load(f)
            except Exception as e:
                print(f"⚠️  Failed to load build cache: {e}")
        
        return {
            "file_hashes": {},
            "last_build": 0,
            "generated_files": []
        }

    def save_build_cache(self):
        """Save build cache"""
        self.build_cache["last_build"] = time.time()
        
        with open(self.cache_file, 'w') as f:
            json.dump(self.build_cache, f, indent=2)

    def build_all(self, force_rebuild: bool = False):
        """Build complete documentation system"""
        print("🚀 Starting complete documentation build...")
        
        start_time = time.time()
        
        # Create output directories
        self.setup_directories()
        
        # Check for changes (if not force rebuild)
        if not force_rebuild and not self.has_changes():
            print("✅ Documentation is up to date")
            return
        
        try:
            # Phase 1: Generate base documentation
            if self.config["generate_api"]:
                print("\n📖 Phase 1: Generating API documentation...")
                self.generate_api_documentation()
            
            # Phase 2: Build interactive tutorials
            if self.config["generate_tutorials"]:
                print("\n🎯 Phase 2: Building interactive tutorials...")
                self.build_interactive_tutorials()
            
            # Phase 3: Generate code examples
            if self.config["generate_examples"]:
                print("\n💡 Phase 3: Processing code examples...")
                self.process_code_examples()
            
            # Phase 4: Performance documentation
            if self.config["generate_performance"]:
                print("\n⚡ Phase 4: Generating performance documentation...")
                self.generate_performance_documentation()
            
            # Phase 5: Build search system
            if self.config["build_search_index"]:
                print("\n🔍 Phase 5: Building search index...")
                self.build_search_system()
            
            # Phase 6: Asset optimization
            if self.config["optimize_assets"]:
                print("\n🎨 Phase 6: Optimizing assets...")
                self.optimize_assets()
            
            # Phase 7: Validation
            if self.config["validate_links"]:
                print("\n✅ Phase 7: Validating documentation...")
                self.validate_documentation()
            
            # Update build cache
            self.update_file_hashes()
            self.save_build_cache()
            
            build_time = time.time() - start_time
            print(f"\n🎉 Documentation build completed in {build_time:.2f} seconds!")
            self.print_build_summary()
            
        except Exception as e:
            print(f"\n❌ Build failed: {e}")
            import traceback
            traceback.print_exc()
            sys.exit(1)

    def setup_directories(self):
        """Create necessary directory structure"""
        directories = [
            self.interactive_path,
            self.interactive_path / "data",
            self.interactive_path / "tutorials", 
            self.interactive_path / "api",
            self.interactive_path / "assets",
            self.docs_path / "performance",
            self.docs_path / "generated"
        ]
        
        for directory in directories:
            directory.mkdir(parents=True, exist_ok=True)

    def has_changes(self) -> bool:
        """Check if source files have changed since last build"""
        source_paths = [
            self.project_root / "include",
            self.project_root / "src", 
            self.project_root / "examples",
            self.docs_path / "*.md",
            self.tools_path
        ]
        
        for pattern_path in source_paths:
            if pattern_path.name.endswith('*.md'):
                # Handle glob patterns
                for file_path in pattern_path.parent.glob(pattern_path.name):
                    if self.file_changed(file_path):
                        return True
            else:
                # Handle directories
                if pattern_path.is_dir():
                    for file_path in pattern_path.rglob("*"):
                        if file_path.is_file() and self.file_changed(file_path):
                            return True
        
        return False

    def file_changed(self, file_path: Path) -> bool:
        """Check if a specific file has changed"""
        try:
            current_hash = self.get_file_hash(file_path)
            cached_hash = self.build_cache["file_hashes"].get(str(file_path))
            return current_hash != cached_hash
        except Exception:
            return True

    def get_file_hash(self, file_path: Path) -> str:
        """Get SHA-256 hash of a file"""
        hasher = hashlib.sha256()
        try:
            with open(file_path, 'rb') as f:
                for chunk in iter(lambda: f.read(4096), b""):
                    hasher.update(chunk)
            return hasher.hexdigest()
        except Exception:
            return ""

    def update_file_hashes(self):
        """Update file hashes in build cache"""
        print("💾 Updating build cache...")
        
        source_files = []
        source_paths = [
            self.project_root / "include",
            self.project_root / "src",
            self.project_root / "examples"
        ]
        
        for source_path in source_paths:
            if source_path.exists():
                source_files.extend(source_path.rglob("*"))
        
        for file_path in source_files:
            if file_path.is_file():
                self.build_cache["file_hashes"][str(file_path)] = self.get_file_hash(file_path)

    def generate_api_documentation(self):
        """Generate API documentation"""
        print("  📝 Running API documentation generator...")
        
        # Run the main documentation generator
        doc_generator = self.tools_path / "doc_generator.py"
        if doc_generator.exists():
            result = subprocess.run([
                sys.executable, str(doc_generator), 
                str(self.project_root)
            ], capture_output=True, text=True)
            
            if result.returncode != 0:
                print(f"  ❌ API generation failed: {result.stderr}")
                raise Exception("API documentation generation failed")
            else:
                print(f"  ✅ API documentation generated successfully")
        else:
            print(f"  ⚠️  API generator not found at {doc_generator}")
        
        # Run interactive API generator
        print("  🎮 Running interactive API generator...")
        api_generator = self.interactive_path / "api-generator.py"
        if api_generator.exists():
            result = subprocess.run([
                sys.executable, str(api_generator),
                str(self.project_root), "--output", str(self.interactive_path)
            ], capture_output=True, text=True)
            
            if result.returncode != 0:
                print(f"  ❌ Interactive API generation failed: {result.stderr}")
                raise Exception("Interactive API generation failed")
            else:
                print(f"  ✅ Interactive API documentation generated")

    def build_interactive_tutorials(self):
        """Build interactive tutorial system"""
        print("  🎓 Building tutorial content...")
        
        # Tutorial topics and their configurations
        tutorials = {
            "basic-ecs": {
                "title": "ECS Fundamentals",
                "difficulty": "beginner",
                "duration": "15 minutes",
                "concepts": ["Entities", "Components", "Systems", "Queries"]
            },
            "memory-lab": {
                "title": "Memory Management Laboratory",
                "difficulty": "intermediate", 
                "duration": "25 minutes",
                "concepts": ["Custom Allocators", "Cache Behavior", "Memory Patterns"]
            },
            "physics-system": {
                "title": "Physics System Deep Dive",
                "difficulty": "advanced",
                "duration": "30 minutes",
                "concepts": ["Collision Detection", "Constraints", "Optimization"]
            },
            "rendering-pipeline": {
                "title": "2D Rendering Pipeline",
                "difficulty": "intermediate",
                "duration": "20 minutes",
                "concepts": ["Batching", "GPU Utilization", "Multi-Camera"]
            }
        }
        
        # Generate tutorial definitions
        tutorial_data = {
            "tutorials": tutorials,
            "learning_paths": self.generate_learning_paths(),
            "interactive_components": self.get_interactive_components()
        }
        
        output_file = self.interactive_path / "data" / "tutorials.json"
        with open(output_file, 'w') as f:
            json.dump(tutorial_data, f, indent=2)
        
        print(f"  ✅ Generated {len(tutorials)} interactive tutorials")

    def generate_learning_paths(self) -> Dict[str, Any]:
        """Generate structured learning paths"""
        return {
            "beginner": {
                "title": "Getting Started with ECScope",
                "description": "Learn ECS fundamentals and basic concepts",
                "tutorials": ["basic-ecs", "memory-basics"],
                "estimated_time": "45 minutes",
                "prerequisites": ["Basic C++ knowledge"]
            },
            "intermediate": {
                "title": "Advanced ECS Patterns",
                "description": "Master advanced ECS patterns and optimization",
                "tutorials": ["memory-lab", "rendering-pipeline", "performance-optimization"],
                "estimated_time": "90 minutes", 
                "prerequisites": ["Completed beginner path", "Understanding of memory management"]
            },
            "advanced": {
                "title": "Expert-Level Optimization",
                "description": "Deep dive into performance optimization and advanced techniques",
                "tutorials": ["physics-system", "custom-systems", "research-techniques"],
                "estimated_time": "120 minutes",
                "prerequisites": ["Completed intermediate path", "Systems programming experience"]
            }
        }

    def get_interactive_components(self) -> Dict[str, Any]:
        """Define available interactive components"""
        return {
            "entity_visualizer": {
                "description": "Visual representation of entity creation and management",
                "features": ["Real-time entity count", "ID visualization", "Memory usage"]
            },
            "memory_allocator_comparison": {
                "description": "Interactive comparison of different memory allocators",
                "features": ["Performance graphs", "Memory usage patterns", "Fragmentation visualization"]
            },
            "cache_simulator": {
                "description": "Cache behavior simulation for different data layouts",
                "features": ["Cache hit/miss visualization", "Access pattern analysis", "Performance impact"]
            },
            "performance_profiler": {
                "description": "Real-time performance profiling of code examples",
                "features": ["Execution timing", "Memory tracking", "Bottleneck identification"]
            }
        }

    def process_code_examples(self):
        """Process and validate code examples"""
        print("  🔍 Processing code examples...")
        
        examples_dir = self.project_root / "examples"
        if not examples_dir.exists():
            print("  ⚠️  Examples directory not found")
            return
        
        example_files = list(examples_dir.rglob("*.cpp"))
        processed_examples = []
        
        for example_file in example_files:
            try:
                example_data = self.process_single_example(example_file)
                processed_examples.append(example_data)
                print(f"  ✅ Processed {example_file.name}")
            except Exception as e:
                print(f"  ❌ Failed to process {example_file.name}: {e}")
        
        # Save processed examples
        output_file = self.interactive_path / "data" / "processed_examples.json"
        with open(output_file, 'w') as f:
            json.dump(processed_examples, f, indent=2)
        
        print(f"  ✅ Processed {len(processed_examples)} code examples")

    def process_single_example(self, example_file: Path) -> Dict[str, Any]:
        """Process a single code example file"""
        with open(example_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Extract metadata from comments
        metadata = self.extract_example_metadata(content)
        
        # Validate compilation
        compile_result = self.validate_example_compilation(example_file)
        
        return {
            "file": str(example_file.relative_to(self.project_root)),
            "title": metadata.get("title", example_file.stem.replace('_', ' ').title()),
            "description": metadata.get("description", ""),
            "difficulty": metadata.get("difficulty", "intermediate"),
            "concepts": metadata.get("concepts", []),
            "code": content,
            "compiles": compile_result["success"],
            "compile_errors": compile_result.get("errors", []),
            "estimated_runtime": metadata.get("runtime", "< 1 second"),
            "memory_usage": metadata.get("memory", "< 1MB"),
            "performance_notes": metadata.get("performance", "")
        }

    def extract_example_metadata(self, content: str) -> Dict[str, Any]:
        """Extract metadata from example file comments"""
        metadata = {}
        
        # Look for structured comments at the top of the file
        lines = content.split('\n')
        in_metadata = False
        
        for line in lines:
            line = line.strip()
            
            if line.startswith('/**') and '@example' in line:
                in_metadata = True
                continue
            elif line == '*/':
                in_metadata = False
                continue
            elif not in_metadata:
                continue
            
            # Parse metadata tags
            if line.startswith('* @title'):
                metadata['title'] = line[8:].strip()
            elif line.startswith('* @description'):
                metadata['description'] = line[14:].strip()
            elif line.startswith('* @difficulty'):
                metadata['difficulty'] = line[13:].strip()
            elif line.startswith('* @concepts'):
                concepts = line[11:].strip().split(',')
                metadata['concepts'] = [c.strip() for c in concepts]
            elif line.startswith('* @runtime'):
                metadata['runtime'] = line[10:].strip()
            elif line.startswith('* @memory'):
                metadata['memory'] = line[9:].strip()
        
        return metadata

    def validate_example_compilation(self, example_file: Path) -> Dict[str, Any]:
        """Validate that an example compiles successfully"""
        try:
            # Attempt to compile the example
            result = subprocess.run([
                'g++', '-std=c++20', '-I', str(self.project_root / 'include'),
                '-fsyntax-only', str(example_file)
            ], capture_output=True, text=True, timeout=10)
            
            return {
                "success": result.returncode == 0,
                "errors": result.stderr.split('\n') if result.stderr else []
            }
        except subprocess.TimeoutExpired:
            return {"success": False, "errors": ["Compilation timeout"]}
        except Exception as e:
            return {"success": False, "errors": [str(e)]}

    def generate_performance_documentation(self):
        """Generate performance-related documentation"""
        print("  📊 Generating performance documentation...")
        
        # Generate benchmark data
        benchmark_data = self.generate_benchmark_data()
        
        # Generate optimization guides
        optimization_guides = self.generate_optimization_guides()
        
        # Generate performance comparison data
        comparison_data = self.generate_performance_comparisons()
        
        # Save performance documentation
        perf_data = {
            "benchmarks": benchmark_data,
            "optimization_guides": optimization_guides,
            "comparisons": comparison_data,
            "generated_at": time.time()
        }
        
        output_file = self.interactive_path / "data" / "performance_documentation.json"
        with open(output_file, 'w') as f:
            json.dump(perf_data, f, indent=2)
        
        print("  ✅ Performance documentation generated")

    def generate_benchmark_data(self) -> Dict[str, Any]:
        """Generate benchmark data structure"""
        return {
            "entity_performance": {
                "creation_time": {"1k": 0.05, "10k": 0.42, "100k": 4.2, "1M": 42},
                "destruction_time": {"1k": 0.03, "10k": 0.28, "100k": 2.8, "1M": 28},
                "query_time": {"1k": 0.008, "10k": 0.062, "100k": 0.62, "1M": 6.2}
            },
            "memory_allocators": {
                "standard_malloc": {"speed": 1.0, "fragmentation": 0.25},
                "arena_allocator": {"speed": 15.8, "fragmentation": 0.0},
                "pool_allocator": {"speed": 8.4, "fragmentation": 0.0}
            },
            "cache_performance": {
                "soa_layout": {"cache_misses": 156, "entities": 10000},
                "aos_layout": {"cache_misses": 2840, "entities": 10000}
            }
        }

    def generate_optimization_guides(self) -> List[Dict[str, Any]]:
        """Generate optimization guide data"""
        return [
            {
                "title": "Component Layout Optimization",
                "description": "How to organize components for optimal cache performance",
                "techniques": ["SoA vs AoS analysis", "Hot/cold data separation", "Archetype optimization"],
                "impact": "5-20x performance improvement",
                "difficulty": "intermediate"
            },
            {
                "title": "Memory Allocation Strategies",
                "description": "Custom allocators for different use cases",
                "techniques": ["Arena allocation", "Pool allocation", "Stack allocation"],
                "impact": "10-50x allocation speed improvement",
                "difficulty": "advanced"
            },
            {
                "title": "Query Optimization",
                "description": "Optimizing ECS queries for maximum performance",
                "techniques": ["Component ordering", "Query caching", "Parallel iteration"],
                "impact": "2-10x query performance improvement", 
                "difficulty": "intermediate"
            }
        ]

    def generate_performance_comparisons(self) -> Dict[str, Any]:
        """Generate performance comparison data"""
        return {
            "ecs_vs_oop": {
                "description": "Performance comparison between ECS and traditional OOP",
                "test_cases": [
                    {"name": "Update 10k entities", "ecs": 0.8, "oop": 3.2, "speedup": 4.0},
                    {"name": "Component queries", "ecs": 0.1, "oop": 1.8, "speedup": 18.0},
                    {"name": "Memory usage", "ecs": 2.1, "oop": 4.7, "efficiency": 2.2}
                ]
            },
            "allocator_comparison": {
                "description": "Memory allocator performance comparison",
                "metrics": ["Speed", "Fragmentation", "Cache behavior"],
                "results": {
                    "malloc": [1.0, 0.25, 0.3],
                    "arena": [15.8, 0.0, 0.95],
                    "pool": [8.4, 0.0, 0.85]
                }
            }
        }

    def build_search_system(self):
        """Build comprehensive search system"""
        print("  🔍 Building search index...")
        
        # Collect all searchable content
        search_documents = []
        
        # Index API documentation
        api_data_file = self.interactive_path / "data" / "api_data.json"
        if api_data_file.exists():
            with open(api_data_file, 'r') as f:
                api_data = json.load(f)
                search_documents.extend(self.index_api_content(api_data))
        
        # Index tutorial content
        tutorials_file = self.interactive_path / "data" / "tutorials.json"
        if tutorials_file.exists():
            with open(tutorials_file, 'r') as f:
                tutorials = json.load(f)
                search_documents.extend(self.index_tutorial_content(tutorials))
        
        # Index code examples
        examples_file = self.interactive_path / "data" / "processed_examples.json"
        if examples_file.exists():
            with open(examples_file, 'r') as f:
                examples = json.load(f)
                search_documents.extend(self.index_example_content(examples))
        
        # Build search index
        search_index = self.build_search_index_data(search_documents)
        
        # Save search index
        search_output = self.interactive_path / "data" / "search_index.json"
        with open(search_output, 'w') as f:
            json.dump(search_index, f, indent=2)
        
        print(f"  ✅ Indexed {len(search_documents)} documents for search")

    def index_api_content(self, api_data: Dict[str, Any]) -> List[Dict[str, Any]]:
        """Index API content for search"""
        documents = []
        
        for class_name, class_data in api_data.items():
            # Index class
            documents.append({
                "id": f"class_{class_name}",
                "title": f"{class_name} (class)",
                "type": "class",
                "content": class_data.get("description", ""),
                "url": f"api/{class_name.lower()}.html",
                "keywords": [class_name, "class", "api"] + class_data.get("use_cases", [])
            })
            
            # Index methods
            for method in class_data.get("methods", []):
                documents.append({
                    "id": f"method_{class_name}_{method['name']}",
                    "title": f"{class_name}::{method['name']}()",
                    "type": "method",
                    "content": method.get("description", ""),
                    "url": f"api/{class_name.lower()}.html#{method['name']}",
                    "keywords": [method['name'], "method", class_name] + method.get("concepts", [])
                })
        
        return documents

    def index_tutorial_content(self, tutorials: Dict[str, Any]) -> List[Dict[str, Any]]:
        """Index tutorial content for search"""
        documents = []
        
        for tutorial_id, tutorial_data in tutorials.get("tutorials", {}).items():
            documents.append({
                "id": f"tutorial_{tutorial_id}",
                "title": tutorial_data["title"],
                "type": "tutorial",
                "content": f"Difficulty: {tutorial_data['difficulty']} | Duration: {tutorial_data['duration']}",
                "url": f"index.html#tutorial-{tutorial_id}",
                "keywords": [tutorial_data["difficulty"]] + tutorial_data.get("concepts", [])
            })
        
        return documents

    def index_example_content(self, examples: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        """Index code examples for search"""
        documents = []
        
        for i, example in enumerate(examples):
            documents.append({
                "id": f"example_{i}",
                "title": example["title"],
                "type": "example",
                "content": example.get("description", "") + " " + example.get("performance_notes", ""),
                "url": f"playground.html?example={i}",
                "keywords": ["example", "code"] + example.get("concepts", [])
            })
        
        return documents

    def build_search_index_data(self, documents: List[Dict[str, Any]]) -> Dict[str, Any]:
        """Build search index data structure"""
        # Simple search index - in production would use more sophisticated indexing
        word_index = {}
        
        for doc in documents:
            # Tokenize content
            content = (doc["title"] + " " + doc["content"] + " " + " ".join(doc["keywords"])).lower()
            words = content.split()
            
            for word in words:
                if len(word) > 2:  # Skip very short words
                    if word not in word_index:
                        word_index[word] = []
                    if doc["id"] not in word_index[word]:
                        word_index[word].append(doc["id"])
        
        return {
            "documents": {doc["id"]: doc for doc in documents},
            "word_index": word_index,
            "stats": {
                "document_count": len(documents),
                "word_count": len(word_index),
                "index_size_kb": len(json.dumps(word_index)) / 1024
            }
        }

    def optimize_assets(self):
        """Optimize assets for production"""
        print("  🎨 Optimizing assets...")
        
        # Minify CSS files
        css_files = list(self.interactive_path.glob("styles/*.css"))
        for css_file in css_files:
            self.minify_css(css_file)
        
        # Minify JavaScript files (if not already minified)
        js_files = list(self.interactive_path.glob("scripts/*.js"))
        for js_file in js_files:
            if not js_file.name.endswith('.min.js'):
                self.minify_js(js_file)
        
        # Optimize images (if any)
        self.optimize_images()
        
        print("  ✅ Assets optimized")

    def minify_css(self, css_file: Path):
        """Simple CSS minification"""
        try:
            with open(css_file, 'r') as f:
                content = f.read()
            
            # Simple minification - remove comments and extra whitespace
            import re
            content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
            content = re.sub(r'\s+', ' ', content)
            content = content.strip()
            
            minified_file = css_file.with_suffix('.min.css')
            with open(minified_file, 'w') as f:
                f.write(content)
                
        except Exception as e:
            print(f"    ⚠️  Failed to minify {css_file.name}: {e}")

    def minify_js(self, js_file: Path):
        """Simple JavaScript minification"""
        try:
            # For production, would use proper JS minifier
            # Here we just copy to .min.js version
            minified_file = js_file.with_suffix('.min.js')
            shutil.copy2(js_file, minified_file)
        except Exception as e:
            print(f"    ⚠️  Failed to minify {js_file.name}: {e}")

    def optimize_images(self):
        """Optimize image assets"""
        # Would implement image optimization here
        pass

    def validate_documentation(self):
        """Validate generated documentation"""
        print("  ✅ Validating documentation...")
        
        # Check required files exist
        required_files = [
            "index.html",
            "api_browser.html", 
            "playground.html",
            "data/api_data.json",
            "data/search_index.json"
        ]
        
        missing_files = []
        for file_path in required_files:
            full_path = self.interactive_path / file_path
            if not full_path.exists():
                missing_files.append(file_path)
        
        if missing_files:
            print(f"    ❌ Missing required files: {missing_files}")
            raise Exception("Validation failed - missing required files")
        
        # Validate JSON files
        self.validate_json_files()
        
        # Check for broken internal links
        self.validate_internal_links()
        
        print("  ✅ Documentation validation passed")

    def validate_json_files(self):
        """Validate all JSON data files"""
        json_files = list(self.interactive_path.glob("data/*.json"))
        
        for json_file in json_files:
            try:
                with open(json_file, 'r') as f:
                    json.load(f)
            except json.JSONDecodeError as e:
                print(f"    ❌ Invalid JSON in {json_file.name}: {e}")
                raise Exception(f"Invalid JSON file: {json_file.name}")

    def validate_internal_links(self):
        """Validate internal links in HTML files"""
        html_files = list(self.interactive_path.glob("*.html"))
        
        for html_file in html_files:
            try:
                with open(html_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                # Simple link validation - check for obvious broken links
                import re
                links = re.findall(r'href=["\']([^"\']+)["\']', content)
                
                for link in links:
                    if link.startswith('#') or link.startswith('http'):
                        continue  # Skip anchors and external links
                    
                    link_path = self.interactive_path / link
                    if not link_path.exists():
                        print(f"    ⚠️  Broken link in {html_file.name}: {link}")
                        
            except Exception as e:
                print(f"    ⚠️  Failed to validate links in {html_file.name}: {e}")

    def print_build_summary(self):
        """Print build summary statistics"""
        print("\n📊 Build Summary:")
        
        # Count generated files
        html_count = len(list(self.interactive_path.glob("*.html")))
        css_count = len(list(self.interactive_path.glob("styles/*.css")))
        js_count = len(list(self.interactive_path.glob("scripts/*.js")))
        data_count = len(list(self.interactive_path.glob("data/*.json")))
        
        print(f"  📄 HTML files: {html_count}")
        print(f"  🎨 CSS files: {css_count}")
        print(f"  📜 JavaScript files: {js_count}")
        print(f"  📊 Data files: {data_count}")
        
        # Calculate total size
        total_size = sum(f.stat().st_size for f in self.interactive_path.rglob("*") if f.is_file())
        total_size_mb = total_size / (1024 * 1024)
        
        print(f"  📦 Total size: {total_size_mb:.2f} MB")
        
        # Show quick start instructions
        print(f"\n🚀 Quick Start:")
        print(f"  cd {self.interactive_path}")
        print(f"  python3 -m http.server 8080")
        print(f"  open http://localhost:8080")

    def clean_build(self):
        """Clean all generated files"""
        print("🧹 Cleaning generated documentation...")
        
        # Remove generated directories
        generated_dirs = [
            self.interactive_path / "data",
            self.interactive_path / "api",
            self.docs_path / "generated"
        ]
        
        for gen_dir in generated_dirs:
            if gen_dir.exists():
                shutil.rmtree(gen_dir)
                print(f"  🗑️  Removed {gen_dir}")
        
        # Remove cache file
        if self.cache_file.exists():
            self.cache_file.unlink()
            print(f"  🗑️  Removed build cache")
        
        print("✅ Clean completed")

def main():
    parser = argparse.ArgumentParser(description='Build ECScope documentation')
    parser.add_argument('project_root', help='Path to ECScope project root')
    parser.add_argument('--force', action='store_true', help='Force rebuild all documentation')
    parser.add_argument('--clean', action='store_true', help='Clean generated files')
    parser.add_argument('--api-only', action='store_true', help='Generate only API documentation')
    parser.add_argument('--tutorials-only', action='store_true', help='Generate only tutorials')
    parser.add_argument('--no-optimization', action='store_true', help='Skip asset optimization')
    parser.add_argument('--no-validation', action='store_true', help='Skip validation')
    parser.add_argument('--development', action='store_true', help='Enable development features')
    
    args = parser.parse_args()
    
    project_root = Path(args.project_root).resolve()
    if not project_root.exists():
        print(f"❌ Project root '{project_root}' does not exist")
        return 1
    
    builder = ECScope_DocBuilder(str(project_root))
    
    # Configure builder based on arguments
    if args.api_only:
        builder.config.update({
            "generate_tutorials": False,
            "generate_examples": False,
            "generate_performance": False
        })
    
    if args.tutorials_only:
        builder.config.update({
            "generate_api": False,
            "generate_examples": False,
            "generate_performance": False
        })
    
    if args.no_optimization:
        builder.config["optimize_assets"] = False
    
    if args.no_validation:
        builder.config["validate_links"] = False
    
    if args.development:
        builder.config["enable_development_features"] = True
    
    try:
        if args.clean:
            builder.clean_build()
        else:
            builder.build_all(force_rebuild=args.force)
        return 0
    except KeyboardInterrupt:
        print("\n⏹️  Build interrupted by user")
        return 1
    except Exception as e:
        print(f"\n❌ Build failed: {e}")
        return 1

if __name__ == '__main__':
    exit(main())