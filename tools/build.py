#!/usr/bin/env python3
"""
ECScope Advanced Build System
============================

This script provides a comprehensive build system for ECScope with support for:
- Multiple build configurations
- Cross-compilation
- Docker builds  
- Package generation
- Performance optimization
- Educational build variants

Usage: python build.py [options] [targets]
"""

import argparse
import os
import sys
import subprocess
import json
import platform
from pathlib import Path
from typing import Dict, List, Optional, Tuple

class ECScopeBuildSystem:
    """Advanced build system for ECScope with educational features"""
    
    def __init__(self):
        self.project_root = Path(__file__).parent.absolute()
        self.build_dir = self.project_root / "build"
        self.config = self.load_build_config()
        
    def load_build_config(self) -> Dict:
        """Load build configuration from JSON file"""
        config_file = self.project_root / "build_config.json"
        if config_file.exists():
            with open(config_file, 'r') as f:
                return json.load(f)
        return self.get_default_config()
    
    def get_default_config(self) -> Dict:
        """Get default build configuration"""
        return {
            "build_types": {
                "debug": {
                    "cmake_build_type": "Debug",
                    "features": {
                        "ECSCOPE_ENABLE_SANITIZERS": "ON",
                        "ECSCOPE_ENABLE_COVERAGE": "ON",
                        "ECSCOPE_DEVELOPMENT_MODE": "ON"
                    }
                },
                "release": {
                    "cmake_build_type": "Release", 
                    "features": {
                        "ECSCOPE_ENABLE_LTO": "ON",
                        "ECSCOPE_ENABLE_ADVANCED_OPTIMIZATIONS": "ON",
                        "ECSCOPE_ENABLE_UNITY_BUILD": "ON"
                    }
                },
                "educational": {
                    "cmake_build_type": "RelWithDebInfo",
                    "features": {
                        "ECSCOPE_EDUCATIONAL_MODE": "ON",
                        "ECSCOPE_BUILD_EXAMPLES": "ON",
                        "ECSCOPE_BUILD_BENCHMARKS": "ON",
                        "ECSCOPE_ENABLE_INSTRUMENTATION": "ON"
                    }
                },
                "performance": {
                    "cmake_build_type": "Release",
                    "features": {
                        "ECSCOPE_ENABLE_LTO": "ON",
                        "ECSCOPE_ENABLE_PGO": "ON",
                        "ECSCOPE_ENABLE_ADVANCED_OPTIMIZATIONS": "ON",
                        "ECSCOPE_ENABLE_SIMD": "ON",
                        "ECSCOPE_ENABLE_AVX512": "ON"
                    }
                },
                "minimal": {
                    "cmake_build_type": "MinSizeRel",
                    "features": {
                        "ECSCOPE_ENABLE_GRAPHICS": "OFF",
                        "ECSCOPE_ENABLE_SCRIPTING": "OFF",
                        "ECSCOPE_BUILD_TESTS": "OFF",
                        "ECSCOPE_BUILD_EXAMPLES": "OFF"
                    }
                }
            },
            "feature_presets": {
                "full": {
                    "ECSCOPE_ENABLE_GRAPHICS": "ON",
                    "ECSCOPE_ENABLE_SIMD": "ON", 
                    "ECSCOPE_ENABLE_JOB_SYSTEM": "ON",
                    "ECSCOPE_ENABLE_3D_PHYSICS": "ON",
                    "ECSCOPE_ENABLE_2D_PHYSICS": "ON",
                    "ECSCOPE_ENABLE_SCRIPTING": "ON",
                    "ECSCOPE_ENABLE_HARDWARE_DETECTION": "ON",
                    "ECSCOPE_BUILD_TESTS": "ON",
                    "ECSCOPE_BUILD_BENCHMARKS": "ON",
                    "ECSCOPE_BUILD_EXAMPLES": "ON"
                },
                "core": {
                    "ECSCOPE_ENABLE_GRAPHICS": "OFF",
                    "ECSCOPE_ENABLE_SIMD": "ON",
                    "ECSCOPE_ENABLE_JOB_SYSTEM": "ON",
                    "ECSCOPE_ENABLE_3D_PHYSICS": "OFF",
                    "ECSCOPE_BUILD_TESTS": "ON"
                },
                "graphics": {
                    "ECSCOPE_ENABLE_GRAPHICS": "ON",
                    "ECSCOPE_ENABLE_2D_PHYSICS": "ON",
                    "ECSCOPE_BUILD_EXAMPLES": "ON"
                }
            },
            "cross_compile_targets": {
                "linux-arm64": {
                    "toolchain": "cmake/toolchains/aarch64-linux-gnu.cmake",
                    "docker_image": "ecscope-arm64-cross"
                },
                "android-arm64": {
                    "toolchain": "cmake/toolchains/android-arm64.cmake", 
                    "docker_image": "ecscope-android-build"
                },
                "windows-x64": {
                    "toolchain": "cmake/toolchains/mingw-w64.cmake",
                    "docker_image": "ecscope-mingw-cross"
                }
            }
        }
    
    def detect_platform(self) -> Tuple[str, str, str]:
        """Detect current platform information"""
        system = platform.system().lower()
        machine = platform.machine().lower()
        
        # Normalize architecture names
        if machine in ['x86_64', 'amd64']:
            arch = 'x64'
        elif machine in ['i386', 'i686', 'x86']:
            arch = 'x86'
        elif machine in ['aarch64', 'arm64']:
            arch = 'arm64'
        elif machine.startswith('arm'):
            arch = 'arm'
        else:
            arch = machine
            
        return system, arch, f"{system}-{arch}"
    
    def get_cmake_generator(self) -> str:
        """Get the best CMake generator for current platform"""
        # Check for Ninja first (fastest)
        if self.command_exists('ninja'):
            return 'Ninja'
        # Check for Make
        elif self.command_exists('make'):
            return 'Unix Makefiles'
        # Windows fallback
        elif platform.system() == 'Windows':
            return 'Visual Studio 16 2019'
        else:
            return 'Unix Makefiles'
    
    def command_exists(self, command: str) -> bool:
        """Check if a command exists in PATH"""
        try:
            subprocess.run([command, '--version'], 
                         capture_output=True, check=True)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False
    
    def run_command(self, cmd: List[str], cwd: Optional[Path] = None, 
                   check: bool = True, capture_output: bool = False) -> subprocess.CompletedProcess:
        """Run a command with proper error handling"""
        print(f"Running: {' '.join(cmd)}")
        if cwd:
            print(f"Working directory: {cwd}")
            
        return subprocess.run(
            cmd, cwd=cwd, check=check, 
            capture_output=capture_output, text=True
        )
    
    def configure_build(self, build_type: str, features: Dict[str, str],
                       cross_compile: Optional[str] = None,
                       generator: Optional[str] = None) -> bool:
        """Configure CMake build"""
        
        # Create build directory
        build_subdir = f"build-{build_type}"
        if cross_compile:
            build_subdir += f"-{cross_compile}"
        
        build_path = self.project_root / build_subdir
        build_path.mkdir(parents=True, exist_ok=True)
        
        # Prepare CMake command
        cmake_args = [
            'cmake',
            str(self.project_root),
            f'-DCMAKE_BUILD_TYPE={self.config["build_types"][build_type]["cmake_build_type"]}'
        ]
        
        # Add generator
        if not generator:
            generator = self.get_cmake_generator()
        cmake_args.extend(['-G', generator])
        
        # Add cross-compilation toolchain
        if cross_compile and cross_compile in self.config['cross_compile_targets']:
            toolchain_path = self.project_root / self.config['cross_compile_targets'][cross_compile]['toolchain']
            if toolchain_path.exists():
                cmake_args.append(f'-DCMAKE_TOOLCHAIN_FILE={toolchain_path}')
        
        # Add build type features
        type_features = self.config['build_types'][build_type].get('features', {})
        for key, value in type_features.items():
            cmake_args.append(f'-D{key}={value}')
        
        # Add custom features
        for key, value in features.items():
            cmake_args.append(f'-D{key}={value}')
        
        try:
            result = self.run_command(cmake_args, cwd=build_path)
            print(f"✓ Configuration successful for {build_type} build")
            return True
        except subprocess.CalledProcessError as e:
            print(f"✗ Configuration failed: {e}")
            return False
    
    def build_targets(self, build_type: str, targets: List[str] = None,
                     cross_compile: Optional[str] = None, 
                     parallel_jobs: Optional[int] = None) -> bool:
        """Build specified targets"""
        
        build_subdir = f"build-{build_type}"
        if cross_compile:
            build_subdir += f"-{cross_compile}"
            
        build_path = self.project_root / build_subdir
        
        if not build_path.exists():
            print(f"✗ Build directory {build_path} does not exist. Run configure first.")
            return False
        
        # Prepare build command
        build_args = ['cmake', '--build', str(build_path)]
        
        if parallel_jobs:
            build_args.extend(['--parallel', str(parallel_jobs)])
        elif self.get_cmake_generator() == 'Ninja':
            # Ninja auto-detects optimal parallel jobs
            pass
        else:
            # Use all available cores for make
            import multiprocessing
            build_args.extend(['--parallel', str(multiprocessing.cpu_count())])
        
        # Add specific targets
        if targets:
            build_args.append('--target')
            build_args.extend(targets)
        
        try:
            result = self.run_command(build_args)
            print(f"✓ Build successful for {build_type}")
            return True
        except subprocess.CalledProcessError as e:
            print(f"✗ Build failed: {e}")
            return False
    
    def run_tests(self, build_type: str, test_labels: List[str] = None,
                 cross_compile: Optional[str] = None) -> bool:
        """Run tests with CTest"""
        
        build_subdir = f"build-{build_type}"
        if cross_compile:
            build_subdir += f"-{cross_compile}"
            
        build_path = self.project_root / build_subdir
        
        if not build_path.exists():
            print(f"✗ Build directory {build_path} does not exist")
            return False
        
        ctest_args = ['ctest', '--output-on-failure', '--verbose']
        
        if test_labels:
            for label in test_labels:
                ctest_args.extend(['-L', label])
        
        try:
            result = self.run_command(ctest_args, cwd=build_path)
            print(f"✓ Tests passed for {build_type}")
            return True
        except subprocess.CalledProcessError as e:
            print(f"✗ Tests failed: {e}")
            return False
    
    def generate_package(self, build_type: str, package_types: List[str] = None) -> bool:
        """Generate installation packages"""
        
        build_path = self.project_root / f"build-{build_type}"
        
        if not build_path.exists():
            print(f"✗ Build directory {build_path} does not exist")
            return False
        
        if not package_types:
            system, arch, _ = self.detect_platform()
            if system == 'linux':
                package_types = ['DEB', 'RPM', 'TGZ']
            elif system == 'windows':
                package_types = ['WIX', 'ZIP']
            elif system == 'darwin':
                package_types = ['DragNDrop', 'TGZ']
            else:
                package_types = ['TGZ']
        
        success = True
        for package_type in package_types:
            try:
                result = self.run_command([
                    'cpack', '-G', package_type
                ], cwd=build_path)
                print(f"✓ {package_type} package generated")
            except subprocess.CalledProcessError as e:
                print(f"✗ {package_type} package failed: {e}")
                success = False
        
        return success
    
    def docker_build(self, image_type: str = 'development') -> bool:
        """Build ECScope using Docker"""
        
        docker_compose_file = self.project_root / 'cmake' / 'docker' / 'docker-compose.yml'
        
        if not docker_compose_file.exists():
            print("✗ Docker compose file not found")
            return False
        
        try:
            # Build the image
            result = self.run_command([
                'docker-compose', '-f', str(docker_compose_file),
                'build', f'ecscope-{image_type}'
            ])
            
            # Run the build
            result = self.run_command([
                'docker-compose', '-f', str(docker_compose_file),
                'run', '--rm', f'ecscope-{image_type}'
            ])
            
            print(f"✓ Docker build successful for {image_type}")
            return True
        except subprocess.CalledProcessError as e:
            print(f"✗ Docker build failed: {e}")
            return False
    
    def clean_build(self, build_type: str = None) -> bool:
        """Clean build directories"""
        
        if build_type:
            build_dirs = [self.project_root / f"build-{build_type}"]
        else:
            build_dirs = list(self.project_root.glob("build-*"))
            build_dirs.append(self.project_root / "build")
        
        for build_dir in build_dirs:
            if build_dir.exists() and build_dir.is_dir():
                import shutil
                shutil.rmtree(build_dir)
                print(f"✓ Cleaned {build_dir}")
        
        return True
    
    def benchmark_build_times(self, iterations: int = 3) -> Dict:
        """Benchmark different build configurations"""
        
        import time
        results = {}
        
        build_types = ['debug', 'release', 'educational']
        
        for build_type in build_types:
            print(f"\nBenchmarking {build_type} build...")
            times = []
            
            for i in range(iterations):
                print(f"  Iteration {i+1}/{iterations}")
                
                # Clean previous build
                self.clean_build(build_type)
                
                # Configure
                start_time = time.time()
                if not self.configure_build(build_type, {}):
                    continue
                    
                # Build
                if not self.build_targets(build_type):
                    continue
                    
                end_time = time.time()
                build_time = end_time - start_time
                times.append(build_time)
                print(f"    Build time: {build_time:.2f}s")
            
            if times:
                avg_time = sum(times) / len(times)
                results[build_type] = {
                    'times': times,
                    'average': avg_time,
                    'min': min(times),
                    'max': max(times)
                }
                print(f"  Average time: {avg_time:.2f}s")
        
        return results


def main():
    parser = argparse.ArgumentParser(
        description='ECScope Advanced Build System',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  python build.py --type educational --features full
  python build.py --type release --cross linux-arm64 --package
  python build.py --docker development
  python build.py --benchmark-builds
  python build.py --clean-all
        '''
    )
    
    parser.add_argument('--type', '-t', 
                       choices=['debug', 'release', 'educational', 'performance', 'minimal'],
                       default='educational',
                       help='Build type (default: educational)')
    
    parser.add_argument('--features', '-f',
                       choices=['full', 'core', 'graphics'],
                       help='Feature preset to enable')
    
    parser.add_argument('--cross', '-c',
                       help='Cross-compile target (linux-arm64, android-arm64, windows-x64)')
    
    parser.add_argument('--generator', '-g',
                       help='CMake generator (Ninja, Make, etc.)')
    
    parser.add_argument('--parallel', '-j', type=int,
                       help='Number of parallel build jobs')
    
    parser.add_argument('--targets', nargs='+',
                       help='Specific targets to build')
    
    parser.add_argument('--test', action='store_true',
                       help='Run tests after building')
    
    parser.add_argument('--test-labels', nargs='+',
                       help='Test labels to run (unit, integration, performance)')
    
    parser.add_argument('--package', action='store_true',
                       help='Generate installation packages')
    
    parser.add_argument('--package-types', nargs='+',
                       help='Package types (DEB, RPM, WIX, etc.)')
    
    parser.add_argument('--docker', 
                       choices=['development', 'runtime', 'testing', 'alpine'],
                       help='Build using Docker')
    
    parser.add_argument('--clean', action='store_true',
                       help='Clean build directory before building')
    
    parser.add_argument('--clean-all', action='store_true',
                       help='Clean all build directories')
    
    parser.add_argument('--benchmark-builds', action='store_true',
                       help='Benchmark build times for different configurations')
    
    parser.add_argument('--config-only', action='store_true',
                       help='Only configure, don\'t build')
    
    args = parser.parse_args()
    
    # Create build system instance
    build_system = ECScopeBuildSystem()
    
    # Handle special commands
    if args.clean_all:
        return build_system.clean_build()
    
    if args.benchmark_builds:
        results = build_system.benchmark_build_times()
        print("\nBuild Time Benchmark Results:")
        print("=" * 50)
        for build_type, data in results.items():
            print(f"{build_type.capitalize()}: {data['average']:.2f}s avg ({data['min']:.2f}s - {data['max']:.2f}s)")
        return True
    
    if args.docker:
        return build_system.docker_build(args.docker)
    
    # Clean if requested
    if args.clean:
        build_system.clean_build(args.type)
    
    # Get features to enable
    features = {}
    if args.features:
        features.update(build_system.config['feature_presets'][args.features])
    
    # Configure build
    if not build_system.configure_build(
        args.type, features, args.cross, args.generator
    ):
        return False
    
    # Build if not config-only
    if not args.config_only:
        if not build_system.build_targets(
            args.type, args.targets, args.cross, args.parallel
        ):
            return False
    
    # Run tests if requested
    if args.test:
        if not build_system.run_tests(args.type, args.test_labels, args.cross):
            return False
    
    # Generate packages if requested
    if args.package:
        if not build_system.generate_package(args.type, args.package_types):
            return False
    
    print(f"\n✅ ECScope build complete! Build type: {args.type}")
    return True


if __name__ == '__main__':
    try:
        success = main()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\n❌ Build interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Build failed with error: {e}")
        sys.exit(1)