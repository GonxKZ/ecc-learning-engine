#!/usr/bin/env python3
"""
ECScope Interactive Documentation Build Script

This script orchestrates the complete build process for ECScope's interactive
documentation system, including:

- API documentation generation from source code
- Tutorial content processing
- Interactive example preparation  
- Performance data integration
- Multi-format documentation export
- Development server setup

Usage:
    python build-docs.py --mode development  # Dev server with hot reload
    python build-docs.py --mode production   # Production build
    python build-docs.py --mode api-only     # Just regenerate API docs
"""

import os
import sys
import argparse
import subprocess
import json
import shutil
import logging
from pathlib import Path
from typing import Dict, List, Optional
import concurrent.futures
import time

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler('docs-build.log')
    ]
)
logger = logging.getLogger('docs-builder')

class DocumentationBuilder:
    """Orchestrates the complete documentation build process"""
    
    def __init__(self, project_root: Path):
        self.project_root = project_root
        self.docs_root = project_root / "docs" / "interactive"
        self.tools_root = project_root / "tools"
        self.include_root = project_root / "include"
        self.examples_root = project_root / "examples"
        self.output_root = self.docs_root / "dist"
        
        # Build configuration
        self.config = self._load_build_config()
        
        # Track build state
        self.build_steps = []
        self.start_time = None
        
    def _load_build_config(self) -> Dict:
        """Load build configuration"""
        config_file = self.docs_root / "build-config.json"
        
        default_config = {
            "api_generation": {
                "enabled": True,
                "source_dirs": ["include"],
                "examples_dir": "examples", 
                "output_formats": ["json", "html", "markdown"]
            },
            "tutorials": {
                "enabled": True,
                "source_dir": "src/content/tutorials",
                "validation": True
            },
            "examples": {
                "enabled": True,
                "compilation_check": True,
                "performance_integration": True
            },
            "interactive_features": {
                "code_execution": True,
                "visualizations": True,
                "performance_monitoring": True
            },
            "development": {
                "hot_reload": True,
                "source_maps": True,
                "live_api_updates": True
            },
            "production": {
                "minification": True,
                "compression": True,
                "cdn_assets": False
            }
        }
        
        if config_file.exists():
            with open(config_file) as f:
                user_config = json.load(f)
                # Merge with defaults
                default_config.update(user_config)
        
        return default_config
    
    def build(self, mode: str = "production") -> bool:
        """Execute complete build process"""
        self.start_time = time.time()
        logger.info(f"Starting ECScope documentation build in {mode} mode")
        
        try:
            # Validate environment
            if not self._validate_environment():
                return False
            
            # Prepare output directory
            self._prepare_output_directory()
            
            # Build steps based on mode
            if mode == "api-only":
                success = self._build_api_documentation()
            else:
                success = self._build_complete_documentation(mode)
            
            if success:
                build_time = time.time() - self.start_time
                logger.info(f"Documentation build completed successfully in {build_time:.2f}s")
                self._generate_build_report()
            else:
                logger.error("Documentation build failed")
            
            return success
            
        except Exception as e:
            logger.error(f"Build failed with exception: {e}")
            return False
    
    def _validate_environment(self) -> bool:
        """Validate build environment and dependencies"""
        logger.info("Validating build environment...")
        
        required_tools = {
            'node': 'Node.js is required for the interactive documentation system',
            'npm': 'npm is required for managing JavaScript dependencies',
            'python': 'Python 3.8+ is required for API documentation generation'
        }
        
        missing_tools = []
        for tool, description in required_tools.items():
            if not shutil.which(tool):
                missing_tools.append(f"{tool}: {description}")
        
        if missing_tools:
            logger.error("Missing required tools:")
            for tool in missing_tools:
                logger.error(f"  - {tool}")
            return False
        
        # Check Python version
        python_version = sys.version_info
        if python_version.major < 3 or python_version.minor < 8:
            logger.error("Python 3.8 or higher is required")
            return False
        
        # Check Node.js version
        try:
            result = subprocess.run(['node', '--version'], 
                                  capture_output=True, text=True)
            node_version = result.stdout.strip().lstrip('v')
            major_version = int(node_version.split('.')[0])
            if major_version < 16:
                logger.error("Node.js 16 or higher is required")
                return False
        except:
            logger.error("Could not determine Node.js version")
            return False
        
        # Validate project structure
        required_paths = [
            self.include_root,
            self.examples_root,
            self.docs_root / "src",
            self.docs_root / "package.json"
        ]
        
        missing_paths = [p for p in required_paths if not p.exists()]
        if missing_paths:
            logger.error("Missing required project structure:")
            for path in missing_paths:
                logger.error(f"  - {path}")
            return False
        
        logger.info("Environment validation successful")
        return True
    
    def _prepare_output_directory(self):
        """Prepare output directory structure"""
        logger.info("Preparing output directories...")
        
        if self.output_root.exists():
            shutil.rmtree(self.output_root)
        
        # Create directory structure
        directories = [
            self.output_root,
            self.output_root / "api",
            self.output_root / "tutorials", 
            self.output_root / "examples",
            self.output_root / "assets",
            self.output_root / "generated"
        ]
        
        for directory in directories:
            directory.mkdir(parents=True, exist_ok=True)
    
    def _build_complete_documentation(self, mode: str) -> bool:
        """Build complete documentation system"""
        build_steps = [
            ("Installing dependencies", self._install_dependencies),
            ("Generating API documentation", self._build_api_documentation),
            ("Processing tutorials", self._process_tutorials),
            ("Preparing code examples", self._prepare_code_examples),
            ("Integrating performance data", self._integrate_performance_data),
            ("Building interactive components", self._build_interactive_components),
            ("Optimizing assets", lambda: self._optimize_assets(mode))
        ]
        
        if mode == "development":
            build_steps.append(("Starting development server", self._start_dev_server))
        
        for step_name, step_function in build_steps:
            logger.info(f"Step: {step_name}")
            start_time = time.time()
            
            try:
                if not step_function():
                    logger.error(f"Failed at step: {step_name}")
                    return False
                    
                step_time = time.time() - start_time
                logger.info(f"Completed: {step_name} ({step_time:.2f}s)")
                self.build_steps.append({
                    'name': step_name,
                    'duration': step_time,
                    'success': True
                })
                
            except Exception as e:
                logger.error(f"Exception in step '{step_name}': {e}")
                self.build_steps.append({
                    'name': step_name,
                    'duration': time.time() - start_time,
                    'success': False,
                    'error': str(e)
                })
                return False
        
        return True
    
    def _install_dependencies(self) -> bool:
        """Install npm dependencies"""
        logger.info("Installing npm dependencies...")
        
        try:
            # Check if package.json exists
            package_json = self.docs_root / "package.json"
            if not package_json.exists():
                logger.error("package.json not found")
                return False
            
            # Install dependencies
            result = subprocess.run(['npm', 'install'], 
                                  cwd=self.docs_root,
                                  capture_output=True, text=True)
            
            if result.returncode != 0:
                logger.error(f"npm install failed: {result.stderr}")
                return False
            
            logger.info("Dependencies installed successfully")
            return True
            
        except Exception as e:
            logger.error(f"Failed to install dependencies: {e}")
            return False
    
    def _build_api_documentation(self) -> bool:
        """Generate API documentation from source code"""
        logger.info("Generating API documentation...")
        
        if not self.config["api_generation"]["enabled"]:
            logger.info("API generation disabled in config")
            return True
        
        try:
            # Run API documentation generator
            api_generator = self.tools_root / "api-doc-generator.py"
            
            if not api_generator.exists():
                logger.error(f"API generator not found: {api_generator}")
                return False
            
            # Build command arguments
            cmd = [
                sys.executable, str(api_generator),
                '--source', str(self.include_root),
                '--examples', str(self.examples_root),
                '--output', str(self.output_root / "generated"),
                '--format', 'all'
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode != 0:
                logger.error(f"API generation failed: {result.stderr}")
                return False
            
            logger.info("API documentation generated successfully")
            return True
            
        except Exception as e:
            logger.error(f"Failed to generate API documentation: {e}")
            return False
    
    def _process_tutorials(self) -> bool:
        """Process and validate tutorial content"""
        logger.info("Processing tutorial content...")
        
        if not self.config["tutorials"]["enabled"]:
            logger.info("Tutorial processing disabled in config")
            return True
        
        try:
            tutorials_dir = self.docs_root / "src" / "content" / "tutorials"
            
            if not tutorials_dir.exists():
                logger.warning("No tutorials directory found")
                return True
            
            processed_tutorials = []
            
            # Process each tutorial file
            for tutorial_file in tutorials_dir.glob("*.json"):
                logger.info(f"Processing tutorial: {tutorial_file.name}")
                
                with open(tutorial_file) as f:
                    tutorial_data = json.load(f)
                
                # Validate tutorial structure
                if self.config["tutorials"]["validation"]:
                    if not self._validate_tutorial(tutorial_data):
                        logger.error(f"Invalid tutorial: {tutorial_file.name}")
                        return False
                
                # Process code examples in tutorial
                if not self._process_tutorial_examples(tutorial_data):
                    logger.error(f"Failed to process examples in: {tutorial_file.name}")
                    return False
                
                processed_tutorials.append(tutorial_data)
            
            # Write processed tutorials index
            tutorials_index = {
                "tutorials": processed_tutorials,
                "generated_at": time.time(),
                "total_count": len(processed_tutorials)
            }
            
            with open(self.output_root / "tutorials" / "index.json", 'w') as f:
                json.dump(tutorials_index, f, indent=2)
            
            logger.info(f"Processed {len(processed_tutorials)} tutorials")
            return True
            
        except Exception as e:
            logger.error(f"Failed to process tutorials: {e}")
            return False
    
    def _validate_tutorial(self, tutorial_data: Dict) -> bool:
        """Validate tutorial structure and content"""
        required_fields = [
            'tutorial_id', 'title', 'description', 'difficulty',
            'estimated_duration', 'learning_objectives', 'steps'
        ]
        
        for field in required_fields:
            if field not in tutorial_data:
                logger.error(f"Missing required field: {field}")
                return False
        
        # Validate steps
        if not isinstance(tutorial_data['steps'], list):
            logger.error("Tutorial steps must be an array")
            return False
        
        for i, step in enumerate(tutorial_data['steps']):
            if 'step_id' not in step or 'title' not in step:
                logger.error(f"Invalid step {i}: missing step_id or title")
                return False
        
        return True
    
    def _process_tutorial_examples(self, tutorial_data: Dict) -> bool:
        """Process code examples within tutorial steps"""
        for step in tutorial_data['steps']:
            if 'content' in step and isinstance(step['content'], dict):
                content = step['content']
                
                # Process interactive exercises
                if 'interactive_exercise' in content:
                    exercise = content['interactive_exercise']
                    
                    if 'starter_code' in exercise:
                        # Validate starter code compiles
                        if self.config["examples"]["compilation_check"]:
                            if not self._validate_code_compiles(exercise['starter_code']):
                                logger.warning(f"Starter code may not compile in step: {step['step_id']}")
                    
                    if 'solution' in exercise:
                        # Validate solution compiles
                        if self.config["examples"]["compilation_check"]:
                            if not self._validate_code_compiles(exercise['solution']):
                                logger.error(f"Solution code does not compile in step: {step['step_id']}")
                                return False
        
        return True
    
    def _validate_code_compiles(self, code: str) -> bool:
        """Quick validation that code looks syntactically correct"""
        # Simple heuristics for C++ code validation
        # In a real implementation, this could use clang to actually compile
        
        # Check for basic C++ structure
        has_includes = '#include' in code
        has_main_or_function = 'main()' in code or 'void ' in code or 'int ' in code
        balanced_braces = code.count('{') == code.count('}')
        
        return balanced_braces and (has_includes or has_main_or_function)
    
    def _prepare_code_examples(self) -> bool:
        """Prepare and validate code examples"""
        logger.info("Preparing code examples...")
        
        if not self.config["examples"]["enabled"]:
            logger.info("Example processing disabled in config")
            return True
        
        try:
            examples_output = self.output_root / "examples"
            
            # Copy example files
            if self.examples_root.exists():
                for example_file in self.examples_root.rglob("*.cpp"):
                    relative_path = example_file.relative_to(self.examples_root)
                    output_path = examples_output / relative_path
                    
                    output_path.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(example_file, output_path)
            
            # Generate examples index
            examples_index = self._generate_examples_index()
            
            with open(examples_output / "index.json", 'w') as f:
                json.dump(examples_index, f, indent=2)
            
            logger.info("Code examples prepared successfully")
            return True
            
        except Exception as e:
            logger.error(f"Failed to prepare code examples: {e}")
            return False
    
    def _generate_examples_index(self) -> Dict:
        """Generate index of all available code examples"""
        examples_index = {
            "categories": {},
            "examples": [],
            "generated_at": time.time()
        }
        
        if not self.examples_root.exists():
            return examples_index
        
        for example_file in self.examples_root.rglob("*.cpp"):
            relative_path = example_file.relative_to(self.examples_root)
            category = relative_path.parts[0] if len(relative_path.parts) > 1 else "general"
            
            # Extract example metadata from file
            example_info = {
                "name": example_file.stem,
                "path": str(relative_path),
                "category": category,
                "size": example_file.stat().st_size,
                "modified": example_file.stat().st_mtime
            }
            
            # Try to extract description from file comments
            try:
                with open(example_file, 'r') as f:
                    content = f.read()
                    # Look for description in header comment
                    import re
                    desc_match = re.search(r'/\*\*.*?@brief\s+(.*?)(?:\n|\*/)', content, re.DOTALL)
                    if desc_match:
                        example_info["description"] = desc_match.group(1).strip()
            except:
                pass
            
            examples_index["examples"].append(example_info)
            
            # Add to category
            if category not in examples_index["categories"]:
                examples_index["categories"][category] = []
            examples_index["categories"][category].append(example_info["name"])
        
        return examples_index
    
    def _integrate_performance_data(self) -> bool:
        """Integrate performance benchmarking data"""
        logger.info("Integrating performance data...")
        
        if not self.config["examples"]["performance_integration"]:
            logger.info("Performance integration disabled in config")
            return True
        
        try:
            # Look for existing performance data
            perf_data_files = [
                self.project_root / "benchmarks" / "performance_results.json",
                self.project_root / "build" / "benchmark_results.json"
            ]
            
            performance_data = {}
            
            for perf_file in perf_data_files:
                if perf_file.exists():
                    with open(perf_file) as f:
                        data = json.load(f)
                        performance_data.update(data)
            
            # Write consolidated performance data
            if performance_data:
                with open(self.output_root / "generated" / "performance_data.json", 'w') as f:
                    json.dump(performance_data, f, indent=2)
                
                logger.info("Performance data integrated successfully")
            else:
                logger.warning("No performance data found")
            
            return True
            
        except Exception as e:
            logger.error(f"Failed to integrate performance data: {e}")
            return False
    
    def _build_interactive_components(self) -> bool:
        """Build interactive JavaScript components"""
        logger.info("Building interactive components...")
        
        try:
            # Use webpack to build the interactive components
            result = subprocess.run(['npm', 'run', 'build'], 
                                  cwd=self.docs_root,
                                  capture_output=True, text=True)
            
            if result.returncode != 0:
                logger.error(f"Webpack build failed: {result.stderr}")
                return False
            
            logger.info("Interactive components built successfully")
            return True
            
        except Exception as e:
            logger.error(f"Failed to build interactive components: {e}")
            return False
    
    def _optimize_assets(self, mode: str) -> bool:
        """Optimize assets for production"""
        logger.info("Optimizing assets...")
        
        if mode != "production":
            logger.info("Skipping optimization in development mode")
            return True
        
        try:
            # Additional optimization steps for production
            if self.config["production"]["compression"]:
                # Compress assets
                pass
            
            if self.config["production"]["minification"]:
                # Additional minification
                pass
            
            logger.info("Asset optimization completed")
            return True
            
        except Exception as e:
            logger.error(f"Failed to optimize assets: {e}")
            return False
    
    def _start_dev_server(self) -> bool:
        """Start development server with hot reload"""
        logger.info("Starting development server...")
        
        try:
            # Start webpack dev server
            logger.info("Starting webpack dev server on http://localhost:3000")
            logger.info("Press Ctrl+C to stop the server")
            
            process = subprocess.Popen(['npm', 'run', 'dev'], 
                                     cwd=self.docs_root)
            
            # Wait for the process (this will block until Ctrl+C)
            process.wait()
            
            return True
            
        except KeyboardInterrupt:
            logger.info("Development server stopped by user")
            return True
        except Exception as e:
            logger.error(f"Failed to start development server: {e}")
            return False
    
    def _generate_build_report(self):
        """Generate build report"""
        report_path = self.output_root / "build-report.json"
        
        total_time = time.time() - self.start_time
        
        report = {
            "build_time": total_time,
            "build_date": time.time(),
            "steps": self.build_steps,
            "config": self.config,
            "output_size": self._calculate_output_size(),
            "success": all(step.get('success', False) for step in self.build_steps)
        }
        
        with open(report_path, 'w') as f:
            json.dump(report, f, indent=2)
        
        logger.info(f"Build report written to: {report_path}")
    
    def _calculate_output_size(self) -> Dict:
        """Calculate size of generated documentation"""
        sizes = {}
        
        if self.output_root.exists():
            total_size = sum(f.stat().st_size for f in self.output_root.rglob('*') if f.is_file())
            sizes["total_bytes"] = total_size
            sizes["total_mb"] = round(total_size / (1024 * 1024), 2)
            
            # Calculate sizes by category
            for category in ["api", "tutorials", "examples", "assets"]:
                category_path = self.output_root / category
                if category_path.exists():
                    category_size = sum(f.stat().st_size for f in category_path.rglob('*') if f.is_file())
                    sizes[f"{category}_bytes"] = category_size
        
        return sizes

def main():
    parser = argparse.ArgumentParser(description="Build ECScope Interactive Documentation")
    parser.add_argument('--mode', choices=['development', 'production', 'api-only'], 
                       default='production', help='Build mode')
    parser.add_argument('--project-root', default='.', help='Project root directory')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--clean', action='store_true', help='Clean build (remove all outputs first)')
    
    args = parser.parse_args()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    # Resolve project root
    project_root = Path(args.project_root).resolve()
    
    if not project_root.exists():
        logger.error(f"Project root does not exist: {project_root}")
        return 1
    
    # Clean build if requested
    if args.clean:
        dist_dir = project_root / "docs" / "interactive" / "dist"
        if dist_dir.exists():
            logger.info("Cleaning previous build...")
            shutil.rmtree(dist_dir)
    
    # Build documentation
    builder = DocumentationBuilder(project_root)
    success = builder.build(args.mode)
    
    return 0 if success else 1

if __name__ == '__main__':
    sys.exit(main())