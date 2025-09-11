#!/usr/bin/env python3
"""
ECScope WebAssembly Build Tool

Professional build system for ECScope WebAssembly deployment with
optimization, testing, and deployment capabilities.
"""

import os
import sys
import subprocess
import argparse
import json
import shutil
import time
from pathlib import Path
from typing import Dict, List, Optional, Tuple
import concurrent.futures
import hashlib

class BuildConfig:
    """Build configuration management"""
    
    def __init__(self, config_file: Optional[str] = None):
        self.config = {
            "project_name": "ECScope",
            "version": "1.0.0",
            "build_dir": "build",
            "dist_dir": "dist",
            "cache_dir": ".build_cache",
            "parallel_jobs": os.cpu_count() or 4,
            "default_target": "release",
            "targets": {
                "debug": {
                    "cmake_args": ["-DECSCOPE_WEB_DEBUG=ON"],
                    "emscripten_flags": ["-g4", "-gsource-map", "-O1"],
                    "optimization_level": "debug"
                },
                "release": {
                    "cmake_args": ["-DECSCOPE_WEB_DEBUG=OFF"],
                    "emscripten_flags": ["-O3", "--closure=1", "-flto"],
                    "optimization_level": "release"
                },
                "profile": {
                    "cmake_args": ["-DECSCOPE_WEB_DEBUG=OFF", "-DECSCOPE_WEB_PROFILE=ON"],
                    "emscripten_flags": ["-O2", "-g", "--profiling"],
                    "optimization_level": "profile"
                },
                "size": {
                    "cmake_args": ["-DECSCOPE_WEB_DEBUG=OFF"],
                    "emscripten_flags": ["-Oz", "--closure=1", "-flto", "--strip-all"],
                    "optimization_level": "size"
                }
            },
            "features": {
                "simd": True,
                "threads": True,
                "webgl2": True,
                "webaudio": True,
                "bulk_memory": True,
                "shared_memory": True
            },
            "assets": {
                "compress": True,
                "formats": ["png", "jpg", "jpeg", "wav", "ogg", "mp3", "json", "gltf", "glb"],
                "output_dir": "assets"
            },
            "deployment": {
                "gzip": True,
                "brotli": True,
                "generate_service_worker": True,
                "generate_manifest": True
            }
        }
        
        if config_file and os.path.exists(config_file):
            with open(config_file, 'r') as f:
                user_config = json.load(f)
                self._merge_config(self.config, user_config)
    
    def _merge_config(self, base: Dict, override: Dict):
        """Recursively merge configuration dictionaries"""
        for key, value in override.items():
            if key in base and isinstance(base[key], dict) and isinstance(value, dict):
                self._merge_config(base[key], value)
            else:
                base[key] = value

class BuildCache:
    """Build cache management for incremental builds"""
    
    def __init__(self, cache_dir: str):
        self.cache_dir = Path(cache_dir)
        self.cache_dir.mkdir(exist_ok=True)
        self.cache_file = self.cache_dir / "build_cache.json"
        self.cache_data = self._load_cache()
    
    def _load_cache(self) -> Dict:
        """Load cache data from file"""
        if self.cache_file.exists():
            try:
                with open(self.cache_file, 'r') as f:
                    return json.load(f)
            except (json.JSONDecodeError, IOError):
                pass
        return {"file_hashes": {}, "build_outputs": {}}
    
    def _save_cache(self):
        """Save cache data to file"""
        try:
            with open(self.cache_file, 'w') as f:
                json.dump(self.cache_data, f, indent=2)
        except IOError:
            print("Warning: Could not save build cache")
    
    def get_file_hash(self, file_path: str) -> str:
        """Calculate SHA-256 hash of file"""
        hasher = hashlib.sha256()
        try:
            with open(file_path, 'rb') as f:
                for chunk in iter(lambda: f.read(4096), b""):
                    hasher.update(chunk)
            return hasher.hexdigest()
        except IOError:
            return ""
    
    def is_file_changed(self, file_path: str) -> bool:
        """Check if file has changed since last build"""
        current_hash = self.get_file_hash(file_path)
        cached_hash = self.cache_data["file_hashes"].get(file_path, "")
        return current_hash != cached_hash
    
    def update_file_hash(self, file_path: str):
        """Update cached hash for file"""
        self.cache_data["file_hashes"][file_path] = self.get_file_hash(file_path)
    
    def is_build_needed(self, sources: List[str], target: str) -> bool:
        """Check if build is needed based on source changes"""
        for source in sources:
            if self.is_file_changed(source):
                return True
        
        # Check if target output exists
        target_info = self.cache_data["build_outputs"].get(target)
        if not target_info:
            return True
        
        output_files = target_info.get("output_files", [])
        for output_file in output_files:
            if not os.path.exists(output_file):
                return True
        
        return False
    
    def update_build_output(self, target: str, output_files: List[str]):
        """Update build output information"""
        self.cache_data["build_outputs"][target] = {
            "timestamp": time.time(),
            "output_files": output_files
        }
        self._save_cache()

class WebAssemblyBuilder:
    """Main WebAssembly build system"""
    
    def __init__(self, config: BuildConfig):
        self.config = config
        self.cache = BuildCache(config.config["cache_dir"])
        self.project_root = Path.cwd()
        self.build_start_time = time.time()
    
    def check_dependencies(self) -> bool:
        """Check if required tools are available"""
        required_tools = ["emcc", "cmake", "python3"]
        optional_tools = ["gzip", "brotli", "node"]
        
        missing_required = []
        missing_optional = []
        
        for tool in required_tools:
            if not shutil.which(tool):
                missing_required.append(tool)
        
        for tool in optional_tools:
            if not shutil.which(tool):
                missing_optional.append(tool)
        
        if missing_required:
            print(f"Error: Missing required tools: {', '.join(missing_required)}")
            print("Please install Emscripten SDK and CMake")
            return False
        
        if missing_optional:
            print(f"Warning: Missing optional tools: {', '.join(missing_optional)}")
            print("Some features may not be available")
        
        return True
    
    def get_source_files(self) -> List[str]:
        """Get list of all source files"""
        source_extensions = [".cpp", ".c", ".hpp", ".h", ".js", ".ts"]
        source_files = []
        
        for ext in source_extensions:
            source_files.extend(
                str(p) for p in self.project_root.rglob(f"*{ext}")
                if not any(part.startswith('.') for part in p.parts)
                and "build" not in p.parts
                and "dist" not in p.parts
            )
        
        return source_files
    
    def configure_cmake(self, target: str, build_dir: Path) -> bool:
        """Configure CMake for target"""
        print(f"Configuring CMake for {target}...")
        
        target_config = self.config.config["targets"][target]
        cmake_args = [
            "emcmake", "cmake",
            "-B", str(build_dir),
            "-S", str(self.project_root),
            "-DCMAKE_BUILD_TYPE=Release"
        ] + target_config["cmake_args"]
        
        # Add feature flags
        features = self.config.config["features"]
        if features.get("simd"):
            cmake_args.append("-DECSCOPE_WEB_SIMD=ON")
        if features.get("threads"):
            cmake_args.append("-DECSCOPE_WEB_THREADS=ON")
        if features.get("webgl2"):
            cmake_args.append("-DECSCOPE_WEB_WEBGL2=ON")
        if features.get("webaudio"):
            cmake_args.append("-DECSCOPE_WEB_WEBAUDIO=ON")
        
        try:
            result = subprocess.run(
                cmake_args,
                cwd=self.project_root,
                capture_output=True,
                text=True,
                timeout=300
            )
            
            if result.returncode != 0:
                print(f"CMake configuration failed:")
                print(result.stderr)
                return False
            
            return True
        
        except subprocess.TimeoutExpired:
            print("CMake configuration timed out")
            return False
        except Exception as e:
            print(f"CMake configuration error: {e}")
            return False
    
    def build_target(self, target: str) -> bool:
        """Build specific target"""
        print(f"Building target: {target}")
        
        build_dir = Path(self.config.config["build_dir"]) / target
        build_dir.mkdir(parents=True, exist_ok=True)
        
        # Check if build is needed
        source_files = self.get_source_files()
        if not self.cache.is_build_needed(source_files, target):
            print(f"Target {target} is up to date, skipping build")
            return True
        
        # Configure CMake
        if not self.configure_cmake(target, build_dir):
            return False
        
        # Build
        print(f"Compiling {target}...")
        build_args = [
            "cmake", "--build", str(build_dir),
            "--parallel", str(self.config.config["parallel_jobs"])
        ]
        
        try:
            result = subprocess.run(
                build_args,
                cwd=self.project_root,
                capture_output=True,
                text=True,
                timeout=1800  # 30 minutes
            )
            
            if result.returncode != 0:
                print(f"Build failed:")
                print(result.stderr)
                return False
            
            # Update cache
            output_files = self._get_build_outputs(build_dir)
            self.cache.update_build_output(target, output_files)
            
            for source_file in source_files:
                self.cache.update_file_hash(source_file)
            
            print(f"Build completed successfully for {target}")
            return True
        
        except subprocess.TimeoutExpired:
            print("Build timed out")
            return False
        except Exception as e:
            print(f"Build error: {e}")
            return False
    
    def _get_build_outputs(self, build_dir: Path) -> List[str]:
        """Get list of build output files"""
        output_files = []
        for pattern in ["*.js", "*.wasm", "*.html", "*.map"]:
            output_files.extend(str(p) for p in build_dir.glob(pattern))
        return output_files
    
    def optimize_outputs(self, target: str) -> bool:
        """Optimize build outputs"""
        print(f"Optimizing outputs for {target}...")
        
        build_dir = Path(self.config.config["build_dir"]) / target
        dist_dir = Path(self.config.config["dist_dir"])
        dist_dir.mkdir(exist_ok=True)
        
        # Copy main files
        for pattern in ["*.js", "*.wasm"]:
            for file_path in build_dir.glob(pattern):
                dest_path = dist_dir / file_path.name
                shutil.copy2(file_path, dest_path)
                print(f"Copied {file_path.name} to dist/")
        
        # Compress files if enabled
        deployment_config = self.config.config["deployment"]
        if deployment_config.get("gzip"):
            self._compress_files(dist_dir, "gzip")
        
        if deployment_config.get("brotli"):
            self._compress_files(dist_dir, "brotli")
        
        # Generate service worker
        if deployment_config.get("generate_service_worker"):
            self._generate_service_worker(dist_dir)
        
        # Generate web app manifest
        if deployment_config.get("generate_manifest"):
            self._generate_manifest(dist_dir)
        
        return True
    
    def _compress_files(self, dist_dir: Path, compression: str):
        """Compress files using specified compression"""
        compressible_extensions = [".js", ".wasm", ".html", ".css", ".json"]
        
        for file_path in dist_dir.iterdir():
            if file_path.suffix in compressible_extensions:
                if compression == "gzip" and shutil.which("gzip"):
                    subprocess.run(["gzip", "-9", "-k", str(file_path)])
                    print(f"Created {file_path.name}.gz")
                elif compression == "brotli" and shutil.which("brotli"):
                    subprocess.run(["brotli", "-9", "-k", str(file_path)])
                    print(f"Created {file_path.name}.br")
    
    def _generate_service_worker(self, dist_dir: Path):
        """Generate service worker for caching"""
        sw_content = '''
// ECScope Service Worker
const CACHE_NAME = 'ecscope-v1';
const urlsToCache = [
  '/',
  '/ecscope.js',
  '/ecscope.wasm'
];

self.addEventListener('install', function(event) {
  event.waitUntil(
    caches.open(CACHE_NAME)
      .then(function(cache) {
        return cache.addAll(urlsToCache);
      })
  );
});

self.addEventListener('fetch', function(event) {
  event.respondWith(
    caches.match(event.request)
      .then(function(response) {
        if (response) {
          return response;
        }
        return fetch(event.request);
      }
    )
  );
});
'''
        
        sw_path = dist_dir / "sw.js"
        with open(sw_path, 'w') as f:
            f.write(sw_content)
        print("Generated service worker")
    
    def _generate_manifest(self, dist_dir: Path):
        """Generate web app manifest"""
        manifest = {
            "name": self.config.config["project_name"],
            "short_name": self.config.config["project_name"],
            "version": self.config.config["version"],
            "start_url": "/",
            "display": "fullscreen",
            "orientation": "landscape",
            "theme_color": "#000000",
            "background_color": "#000000",
            "icons": [
                {
                    "src": "icon-192.png",
                    "sizes": "192x192",
                    "type": "image/png"
                },
                {
                    "src": "icon-512.png",
                    "sizes": "512x512",
                    "type": "image/png"
                }
            ]
        }
        
        manifest_path = dist_dir / "manifest.json"
        with open(manifest_path, 'w') as f:
            json.dump(manifest, f, indent=2)
        print("Generated web app manifest")
    
    def run_tests(self, target: str) -> bool:
        """Run tests for target"""
        print(f"Running tests for {target}...")
        
        # This would integrate with actual test framework
        # For now, just check if files exist and are valid
        
        build_dir = Path(self.config.config["build_dir"]) / target
        
        # Check WebAssembly file
        wasm_files = list(build_dir.glob("*.wasm"))
        if not wasm_files:
            print("Error: No WebAssembly file found")
            return False
        
        wasm_file = wasm_files[0]
        if wasm_file.stat().st_size == 0:
            print("Error: WebAssembly file is empty")
            return False
        
        # Check JavaScript file
        js_files = list(build_dir.glob("*.js"))
        if not js_files:
            print("Error: No JavaScript file found")
            return False
        
        js_file = js_files[0]
        if js_file.stat().st_size == 0:
            print("Error: JavaScript file is empty")
            return False
        
        print("Basic validation tests passed")
        return True
    
    def analyze_build(self, target: str):
        """Analyze build outputs"""
        print(f"Analyzing build for {target}...")
        
        build_dir = Path(self.config.config["build_dir"]) / target
        
        # File size analysis
        for file_path in build_dir.glob("*"):
            if file_path.is_file():
                size_mb = file_path.stat().st_size / (1024 * 1024)
                print(f"  {file_path.name}: {size_mb:.2f} MB")
        
        # Build time analysis
        build_time = time.time() - self.build_start_time
        print(f"Total build time: {build_time:.2f} seconds")
    
    def clean(self, target: Optional[str] = None):
        """Clean build artifacts"""
        if target:
            build_dir = Path(self.config.config["build_dir"]) / target
            if build_dir.exists():
                shutil.rmtree(build_dir)
                print(f"Cleaned {target} build")
        else:
            build_dir = Path(self.config.config["build_dir"])
            if build_dir.exists():
                shutil.rmtree(build_dir)
                print("Cleaned all builds")
            
            dist_dir = Path(self.config.config["dist_dir"])
            if dist_dir.exists():
                shutil.rmtree(dist_dir)
                print("Cleaned dist directory")
            
            cache_dir = Path(self.config.config["cache_dir"])
            if cache_dir.exists():
                shutil.rmtree(cache_dir)
                print("Cleaned build cache")

def main():
    parser = argparse.ArgumentParser(description="ECScope WebAssembly Build Tool")
    parser.add_argument("command", choices=["build", "clean", "test", "analyze", "serve"],
                       help="Build command to execute")
    parser.add_argument("--target", "-t", default="release",
                       choices=["debug", "release", "profile", "size"],
                       help="Build target")
    parser.add_argument("--config", "-c", help="Build configuration file")
    parser.add_argument("--parallel", "-j", type=int, help="Number of parallel jobs")
    parser.add_argument("--no-cache", action="store_true", help="Disable build cache")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    
    args = parser.parse_args()
    
    # Load configuration
    config = BuildConfig(args.config)
    
    if args.parallel:
        config.config["parallel_jobs"] = args.parallel
    
    # Create builder
    builder = WebAssemblyBuilder(config)
    
    # Check dependencies
    if not builder.check_dependencies():
        sys.exit(1)
    
    # Execute command
    if args.command == "build":
        success = builder.build_target(args.target)
        if success:
            builder.optimize_outputs(args.target)
            builder.analyze_build(args.target)
        sys.exit(0 if success else 1)
    
    elif args.command == "clean":
        builder.clean(args.target if args.target != "release" else None)
    
    elif args.command == "test":
        success = builder.run_tests(args.target)
        sys.exit(0 if success else 1)
    
    elif args.command == "analyze":
        builder.analyze_build(args.target)
    
    elif args.command == "serve":
        import http.server
        import socketserver
        
        dist_dir = Path(config.config["dist_dir"])
        if not dist_dir.exists():
            print("Error: dist directory not found. Run 'build' first.")
            sys.exit(1)
        
        os.chdir(dist_dir)
        port = 8080
        
        with socketserver.TCPServer(("", port), http.server.SimpleHTTPRequestHandler) as httpd:
            print(f"Serving ECScope at http://localhost:{port}")
            try:
                httpd.serve_forever()
            except KeyboardInterrupt:
                print("\nServer stopped")

if __name__ == "__main__":
    main()