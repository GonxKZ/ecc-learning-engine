#!/usr/bin/env python3
"""
Cross-Platform Build and Test Automation Script

Comprehensive script for building and testing ECScope across different platforms,
compilers, and configurations. Provides automated compatibility testing and
reporting for Windows, Linux, and macOS.

Author: ECScope Development Team
Version: 1.0.0
"""

import os
import sys
import subprocess
import argparse
import json
import platform
import time
from pathlib import Path
from typing import List, Dict, Optional, Tuple
from dataclasses import dataclass
from enum import Enum

class Platform(Enum):
    WINDOWS = "windows"
    LINUX = "linux"
    MACOS = "macos"
    UNKNOWN = "unknown"

class Compiler(Enum):
    MSVC = "msvc"
    GCC = "gcc"
    CLANG = "clang"
    UNKNOWN = "unknown"

class BuildType(Enum):
    DEBUG = "Debug"
    RELEASE = "Release"
    RELWITHDEBINFO = "RelWithDebInfo"
    MINSIZEREL = "MinSizeRel"

@dataclass
class BuildConfiguration:
    platform: Platform
    compiler: Compiler
    build_type: BuildType
    architecture: str
    cmake_args: List[str]
    test_args: List[str]

@dataclass
class TestResult:
    name: str
    passed: bool
    duration: float
    output: str
    error: Optional[str] = None

@dataclass
class BuildResult:
    configuration: BuildConfiguration
    build_success: bool
    build_time: float
    test_results: List[TestResult]
    build_output: str
    build_error: Optional[str] = None

class CrossPlatformBuilder:
    def __init__(self, source_dir: Path, output_dir: Path):
        self.source_dir = source_dir
        self.output_dir = output_dir
        self.platform = self._detect_platform()
        self.compiler = self._detect_compiler()
        self.architecture = self._detect_architecture()
        
        # Create output directory
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        # Initialize logging
        self.log_file = self.output_dir / "build_log.txt"
        
    def _detect_platform(self) -> Platform:
        """Detect the current platform."""
        system = platform.system().lower()
        if system == "windows":
            return Platform.WINDOWS
        elif system == "linux":
            return Platform.LINUX
        elif system == "darwin":
            return Platform.MACOS
        else:
            return Platform.UNKNOWN
    
    def _detect_compiler(self) -> Compiler:
        """Detect the available compiler."""
        if self.platform == Platform.WINDOWS:
            if self._command_exists("cl"):
                return Compiler.MSVC
            elif self._command_exists("clang++"):
                return Compiler.CLANG
            elif self._command_exists("g++"):
                return Compiler.GCC
        else:
            if self._command_exists("clang++"):
                return Compiler.CLANG
            elif self._command_exists("g++"):
                return Compiler.GCC
        
        return Compiler.UNKNOWN
    
    def _detect_architecture(self) -> str:
        """Detect the system architecture."""
        arch = platform.machine().lower()
        if arch in ["x86_64", "amd64"]:
            return "x64"
        elif arch in ["i386", "i686", "x86"]:
            return "x86"
        elif arch in ["aarch64", "arm64"]:
            return "arm64"
        elif arch.startswith("arm"):
            return "arm"
        else:
            return arch
    
    def _command_exists(self, command: str) -> bool:
        """Check if a command exists in the system PATH."""
        try:
            subprocess.run([command, "--version"], 
                         capture_output=True, check=False)
            return True
        except FileNotFoundError:
            return False
    
    def _log(self, message: str, level: str = "INFO"):
        """Log a message to both console and file."""
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        log_message = f"[{timestamp}] {level}: {message}"
        
        print(log_message)
        
        with open(self.log_file, "a", encoding="utf-8") as f:
            f.write(log_message + "\n")
    
    def generate_build_configurations(self) -> List[BuildConfiguration]:
        """Generate build configurations for the current platform."""
        configurations = []
        
        # Base CMake arguments
        base_cmake_args = [
            "-DECSCOPE_BUILD_TESTS=ON",
            "-DECSCOPE_BUILD_CROSS_PLATFORM_TESTS=ON",
            "-DECSCOPE_BUILD_GUI=ON",
            "-DECSCOPE_BUILD_EXAMPLES=ON",
        ]
        
        # Platform-specific configurations
        if self.platform == Platform.WINDOWS:
            if self.compiler == Compiler.MSVC:
                configurations.extend([
                    BuildConfiguration(
                        platform=self.platform,
                        compiler=self.compiler,
                        build_type=BuildType.DEBUG,
                        architecture=self.architecture,
                        cmake_args=base_cmake_args + [
                            "-G", "Visual Studio 17 2022",
                            "-A", "x64" if self.architecture == "x64" else "Win32",
                            "-DECSCOPE_ENABLE_SANITIZERS=ON"
                        ],
                        test_args=["--parallel", "--output-on-failure"]
                    ),
                    BuildConfiguration(
                        platform=self.platform,
                        compiler=self.compiler,
                        build_type=BuildType.RELEASE,
                        architecture=self.architecture,
                        cmake_args=base_cmake_args + [
                            "-G", "Visual Studio 17 2022",
                            "-A", "x64" if self.architecture == "x64" else "Win32",
                            "-DECSCOPE_ENABLE_ADVANCED_OPTIMIZATIONS=ON"
                        ],
                        test_args=["--parallel", "--output-on-failure"]
                    )
                ])
        
        elif self.platform == Platform.LINUX:
            for compiler in [Compiler.GCC, Compiler.CLANG]:
                if (compiler == Compiler.GCC and self._command_exists("g++")) or \
                   (compiler == Compiler.CLANG and self._command_exists("clang++")):
                    
                    compiler_args = []
                    if compiler == Compiler.GCC:
                        compiler_args = ["-DCMAKE_C_COMPILER=gcc", "-DCMAKE_CXX_COMPILER=g++"]
                    else:
                        compiler_args = ["-DCMAKE_C_COMPILER=clang", "-DCMAKE_CXX_COMPILER=clang++"]
                    
                    configurations.extend([
                        BuildConfiguration(
                            platform=self.platform,
                            compiler=compiler,
                            build_type=BuildType.DEBUG,
                            architecture=self.architecture,
                            cmake_args=base_cmake_args + compiler_args + [
                                "-G", "Ninja",
                                "-DECSCOPE_ENABLE_SANITIZERS=ON",
                                "-DECSCOPE_ENABLE_COVERAGE=ON"
                            ],
                            test_args=["--parallel", "--output-on-failure"]
                        ),
                        BuildConfiguration(
                            platform=self.platform,
                            compiler=compiler,
                            build_type=BuildType.RELEASE,
                            architecture=self.architecture,
                            cmake_args=base_cmake_args + compiler_args + [
                                "-G", "Ninja",
                                "-DECSCOPE_ENABLE_ADVANCED_OPTIMIZATIONS=ON"
                            ],
                            test_args=["--parallel", "--output-on-failure"]
                        )
                    ])
        
        elif self.platform == Platform.MACOS:
            configurations.extend([
                BuildConfiguration(
                    platform=self.platform,
                    compiler=Compiler.CLANG,
                    build_type=BuildType.DEBUG,
                    architecture=self.architecture,
                    cmake_args=base_cmake_args + [
                        "-G", "Xcode",
                        "-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15",
                        "-DECSCOPE_ENABLE_SANITIZERS=ON"
                    ],
                    test_args=["--parallel", "--output-on-failure"]
                ),
                BuildConfiguration(
                    platform=self.platform,
                    compiler=Compiler.CLANG,
                    build_type=BuildType.RELEASE,
                    architecture=self.architecture,
                    cmake_args=base_cmake_args + [
                        "-G", "Xcode",
                        "-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15",
                        "-DECSCOPE_ENABLE_ADVANCED_OPTIMIZATIONS=ON"
                    ],
                    test_args=["--parallel", "--output-on-failure"]
                )
            ])
        
        return configurations
    
    def build_configuration(self, config: BuildConfiguration) -> BuildResult:
        """Build a specific configuration."""
        build_dir = self.output_dir / f"build_{config.platform.value}_{config.compiler.value}_{config.build_type.value}"
        build_dir.mkdir(parents=True, exist_ok=True)
        
        self._log(f"Building configuration: {config.platform.value} {config.compiler.value} {config.build_type.value}")
        
        # Configure
        configure_cmd = [
            "cmake",
            "-S", str(self.source_dir),
            "-B", str(build_dir),
            f"-DCMAKE_BUILD_TYPE={config.build_type.value}"
        ] + config.cmake_args
        
        start_time = time.time()
        
        try:
            self._log(f"Configuring: {' '.join(configure_cmd)}")
            configure_result = subprocess.run(
                configure_cmd,
                capture_output=True,
                text=True,
                cwd=self.source_dir
            )
            
            if configure_result.returncode != 0:
                return BuildResult(
                    configuration=config,
                    build_success=False,
                    build_time=time.time() - start_time,
                    test_results=[],
                    build_output=configure_result.stdout,
                    build_error=configure_result.stderr
                )
            
            # Build
            build_cmd = [
                "cmake",
                "--build", str(build_dir),
                "--config", config.build_type.value,
                "--parallel"
            ]
            
            self._log(f"Building: {' '.join(build_cmd)}")
            build_result = subprocess.run(
                build_cmd,
                capture_output=True,
                text=True,
                cwd=self.source_dir
            )
            
            build_time = time.time() - start_time
            
            if build_result.returncode != 0:
                return BuildResult(
                    configuration=config,
                    build_success=False,
                    build_time=build_time,
                    test_results=[],
                    build_output=build_result.stdout,
                    build_error=build_result.stderr
                )
            
            # Run tests
            test_results = self._run_tests(build_dir, config)
            
            return BuildResult(
                configuration=config,
                build_success=True,
                build_time=build_time,
                test_results=test_results,
                build_output=build_result.stdout + "\n" + configure_result.stdout
            )
            
        except Exception as e:
            return BuildResult(
                configuration=config,
                build_success=False,
                build_time=time.time() - start_time,
                test_results=[],
                build_output="",
                build_error=str(e)
            )
    
    def _run_tests(self, build_dir: Path, config: BuildConfiguration) -> List[TestResult]:
        """Run tests for a build configuration."""
        test_results = []
        
        # Run CTest
        ctest_cmd = [
            "ctest",
            "--build-config", config.build_type.value,
            "--verbose"
        ] + config.test_args
        
        try:
            self._log(f"Running tests: {' '.join(ctest_cmd)}")
            start_time = time.time()
            
            test_result = subprocess.run(
                ctest_cmd,
                capture_output=True,
                text=True,
                cwd=build_dir
            )
            
            duration = time.time() - start_time
            
            # Parse CTest output for individual test results
            # This is a simplified parser - a real implementation would be more robust
            test_results.append(TestResult(
                name="CTest Suite",
                passed=test_result.returncode == 0,
                duration=duration,
                output=test_result.stdout,
                error=test_result.stderr if test_result.returncode != 0 else None
            ))
            
            # Run cross-platform specific tests
            cross_platform_tests = [
                "ecscope_gui_compatibility_test",
                "ecscope_opengl_test",
                "ecscope_glfw_test",
                "ecscope_filesystem_test",
                "ecscope_threading_test",
                "ecscope_display_test"
            ]
            
            for test_name in cross_platform_tests:
                test_executable = build_dir / "bin" / test_name
                if self.platform == Platform.WINDOWS:
                    test_executable = test_executable.with_suffix(".exe")
                
                if test_executable.exists():
                    start_time = time.time()
                    try:
                        result = subprocess.run(
                            [str(test_executable)],
                            capture_output=True,
                            text=True,
                            timeout=300  # 5 minute timeout
                        )
                        
                        test_results.append(TestResult(
                            name=test_name,
                            passed=result.returncode == 0,
                            duration=time.time() - start_time,
                            output=result.stdout,
                            error=result.stderr if result.returncode != 0 else None
                        ))
                        
                    except subprocess.TimeoutExpired:
                        test_results.append(TestResult(
                            name=test_name,
                            passed=False,
                            duration=time.time() - start_time,
                            output="",
                            error="Test timed out after 300 seconds"
                        ))
                    except Exception as e:
                        test_results.append(TestResult(
                            name=test_name,
                            passed=False,
                            duration=time.time() - start_time,
                            output="",
                            error=str(e)
                        ))
            
        except Exception as e:
            test_results.append(TestResult(
                name="Test Execution",
                passed=False,
                duration=0.0,
                output="",
                error=str(e)
            ))
        
        return test_results
    
    def generate_report(self, results: List[BuildResult]) -> Dict:
        """Generate a comprehensive test report."""
        report = {
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "platform": self.platform.value,
            "architecture": self.architecture,
            "summary": {
                "total_configurations": len(results),
                "successful_builds": sum(1 for r in results if r.build_success),
                "failed_builds": sum(1 for r in results if not r.build_success),
                "total_tests": sum(len(r.test_results) for r in results),
                "passed_tests": sum(sum(1 for t in r.test_results if t.passed) for r in results),
                "failed_tests": sum(sum(1 for t in r.test_results if not t.passed) for r in results)
            },
            "configurations": []
        }
        
        for result in results:
            config_report = {
                "platform": result.configuration.platform.value,
                "compiler": result.configuration.compiler.value,
                "build_type": result.configuration.build_type.value,
                "architecture": result.configuration.architecture,
                "build_success": result.build_success,
                "build_time": result.build_time,
                "tests": []
            }
            
            if result.build_error:
                config_report["build_error"] = result.build_error
            
            for test in result.test_results:
                test_report = {
                    "name": test.name,
                    "passed": test.passed,
                    "duration": test.duration
                }
                
                if test.error:
                    test_report["error"] = test.error
                
                config_report["tests"].append(test_report)
            
            report["configurations"].append(config_report)
        
        return report
    
    def run_all_configurations(self) -> List[BuildResult]:
        """Run all build configurations for the current platform."""
        configurations = self.generate_build_configurations()
        results = []
        
        self._log(f"Running {len(configurations)} build configurations")
        
        for i, config in enumerate(configurations, 1):
            self._log(f"[{i}/{len(configurations)}] Starting configuration: {config.platform.value} {config.compiler.value} {config.build_type.value}")
            
            result = self.build_configuration(config)
            results.append(result)
            
            if result.build_success:
                passed_tests = sum(1 for t in result.test_results if t.passed)
                total_tests = len(result.test_results)
                self._log(f"[{i}/{len(configurations)}] ✓ Build successful, tests: {passed_tests}/{total_tests} passed")
            else:
                self._log(f"[{i}/{len(configurations)}] ✗ Build failed", "ERROR")
        
        return results
    
    def save_report(self, report: Dict, filename: str = "cross_platform_test_report.json"):
        """Save the test report to a JSON file."""
        report_file = self.output_dir / filename
        
        with open(report_file, "w", encoding="utf-8") as f:
            json.dump(report, f, indent=2)
        
        self._log(f"Report saved to: {report_file}")
        
        # Also generate a human-readable summary
        summary_file = self.output_dir / "test_summary.txt"
        with open(summary_file, "w", encoding="utf-8") as f:
            f.write("ECScope Cross-Platform Test Summary\n")
            f.write("=" * 40 + "\n\n")
            f.write(f"Platform: {report['platform']}\n")
            f.write(f"Architecture: {report['architecture']}\n")
            f.write(f"Timestamp: {report['timestamp']}\n\n")
            
            summary = report['summary']
            f.write(f"Total Configurations: {summary['total_configurations']}\n")
            f.write(f"Successful Builds: {summary['successful_builds']}\n")
            f.write(f"Failed Builds: {summary['failed_builds']}\n")
            f.write(f"Total Tests: {summary['total_tests']}\n")
            f.write(f"Passed Tests: {summary['passed_tests']}\n")
            f.write(f"Failed Tests: {summary['failed_tests']}\n\n")
            
            for config in report['configurations']:
                f.write(f"Configuration: {config['platform']} {config['compiler']} {config['build_type']}\n")
                f.write(f"  Build: {'✓' if config['build_success'] else '✗'} ({config['build_time']:.2f}s)\n")
                
                if config['tests']:
                    passed = sum(1 for t in config['tests'] if t['passed'])
                    total = len(config['tests'])
                    f.write(f"  Tests: {passed}/{total} passed\n")
                    
                    for test in config['tests']:
                        status = '✓' if test['passed'] else '✗'
                        f.write(f"    {status} {test['name']} ({test['duration']:.2f}s)\n")
                
                f.write("\n")
        
        self._log(f"Summary saved to: {summary_file}")

def main():
    parser = argparse.ArgumentParser(
        description="ECScope Cross-Platform Build and Test Automation"
    )
    parser.add_argument(
        "--source-dir",
        type=Path,
        default=Path.cwd(),
        help="Source directory path (default: current directory)"
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path.cwd() / "cross_platform_tests",
        help="Output directory for build artifacts and reports"
    )
    parser.add_argument(
        "--config-only",
        action="store_true",
        help="Only show available configurations, don't build"
    )
    parser.add_argument(
        "--report-only",
        type=Path,
        help="Generate report from existing results JSON file"
    )
    
    args = parser.parse_args()
    
    if args.report_only:
        # Load existing report and regenerate summary
        if args.report_only.exists():
            with open(args.report_only, "r", encoding="utf-8") as f:
                report = json.load(f)
            
            builder = CrossPlatformBuilder(args.source_dir, args.output_dir)
            builder.save_report(report)
        else:
            print(f"Error: Report file not found: {args.report_only}")
            sys.exit(1)
        return
    
    builder = CrossPlatformBuilder(args.source_dir, args.output_dir)
    
    if args.config_only:
        configurations = builder.generate_build_configurations()
        print(f"Available configurations for {builder.platform.value}:")
        for i, config in enumerate(configurations, 1):
            print(f"  {i}. {config.platform.value} {config.compiler.value} {config.build_type.value} {config.architecture}")
        return
    
    # Run all configurations
    print(f"Starting cross-platform testing on {builder.platform.value} ({builder.architecture})")
    print(f"Output directory: {builder.output_dir}")
    
    results = builder.run_all_configurations()
    report = builder.generate_report(results)
    builder.save_report(report)
    
    # Print summary
    summary = report['summary']
    print("\n" + "=" * 50)
    print("CROSS-PLATFORM TEST SUMMARY")
    print("=" * 50)
    print(f"Configurations: {summary['successful_builds']}/{summary['total_configurations']} successful")
    print(f"Tests: {summary['passed_tests']}/{summary['total_tests']} passed")
    
    if summary['failed_builds'] > 0 or summary['failed_tests'] > 0:
        print("\nSome tests failed. Check the detailed report for more information.")
        sys.exit(1)
    else:
        print("\nAll tests passed successfully!")

if __name__ == "__main__":
    main()