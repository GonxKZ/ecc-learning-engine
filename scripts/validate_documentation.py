#!/usr/bin/env python3
"""
Documentation Validation Script for ECScope
Validates that documentation is accurate and up-to-date.
"""

import json
import argparse
import sys
from pathlib import Path
from typing import Dict, List, Any, Set
import re

class DocumentationValidator:
    def __init__(self):
        self.issues = []
        self.warnings = []
        self.stats = {
            'files_checked': 0,
            'examples_validated': 0,
            'missing_docs': 0,
            'outdated_examples': 0
        }
        
    def validate_source_documentation(self, source_dir: Path) -> bool:
        """Validate source code documentation."""
        print(f"🔍 Validating source documentation in: {source_dir}")
        
        # Find header files
        header_files = list(source_dir.rglob("*.hpp")) + list(source_dir.rglob("*.h"))
        
        for header_file in header_files:
            self._validate_header_documentation(header_file)
            self.stats['files_checked'] += 1
            
        return len(self.issues) == 0
        
    def validate_examples(self, examples_dir: Path) -> bool:
        """Validate example code."""
        print(f"🔍 Validating examples in: {examples_dir}")
        
        if not examples_dir.exists():
            self.warnings.append(f"Examples directory not found: {examples_dir}")
            return True
            
        # Find example files
        example_files = list(examples_dir.rglob("*.cpp")) + list(examples_dir.rglob("*.hpp"))
        
        for example_file in example_files:
            self._validate_example_file(example_file)
            self.stats['examples_validated'] += 1
            
        return len(self.issues) == 0
        
    def _validate_header_documentation(self, header_file: Path):
        """Validate documentation in a header file."""
        try:
            content = header_file.read_text(encoding='utf-8')
            
            # Check for basic documentation patterns
            if not re.search(r'//.*@brief|/\*\*.*@brief', content):
                # Only flag as missing if this appears to be a public API file
                if 'include/ecscope' in str(header_file):
                    self.issues.append({
                        'type': 'missing_documentation',
                        'file': str(header_file),
                        'message': 'Public header file missing @brief documentation'
                    })
                    self.stats['missing_docs'] += 1
                    
            # Check for TODO/FIXME comments that might indicate incomplete implementation
            todos = re.findall(r'//.*(?:TODO|FIXME|XXX).*', content)
            if todos:
                self.warnings.append({
                    'type': 'incomplete_implementation',
                    'file': str(header_file),
                    'message': f'Found {len(todos)} TODO/FIXME comments',
                    'details': todos[:3]  # Show first 3
                })
                
        except Exception as e:
            self.warnings.append({
                'type': 'file_read_error',
                'file': str(header_file),
                'message': f'Could not read file: {e}'
            })
            
    def _validate_example_file(self, example_file: Path):
        """Validate an example file."""
        try:
            content = example_file.read_text(encoding='utf-8')
            
            # Check for basic example structure
            has_main = 'int main(' in content or 'void main(' in content
            has_includes = '#include' in content
            
            if not has_main and example_file.suffix == '.cpp':
                self.issues.append({
                    'type': 'incomplete_example',
                    'file': str(example_file),
                    'message': 'Example file missing main function'
                })
                
            if not has_includes:
                self.issues.append({
                    'type': 'incomplete_example',
                    'file': str(example_file),
                    'message': 'Example file missing includes'
                })
                
            # Check for common ECScope includes to ensure examples are using the API
            ecscope_includes = re.findall(r'#include\s*[<"]([^>"]*ecscope[^>"]*)[>"]', content)
            if not ecscope_includes and 'ecscope' not in content.lower():
                self.warnings.append({
                    'type': 'potential_outdated_example',
                    'file': str(example_file),
                    'message': 'Example may not be using ECScope API'
                })
                self.stats['outdated_examples'] += 1
                
        except Exception as e:
            self.warnings.append({
                'type': 'file_read_error',
                'file': str(example_file),
                'message': f'Could not read file: {e}'
            })
            
    def generate_report(self) -> Dict[str, Any]:
        """Generate validation report."""
        return {
            'timestamp': self._get_timestamp(),
            'status': 'passed' if len(self.issues) == 0 else 'failed',
            'statistics': self.stats,
            'issues': self.issues,
            'warnings': self.warnings,
            'summary': {
                'total_issues': len(self.issues),
                'total_warnings': len(self.warnings),
                'files_with_issues': len(set(issue['file'] for issue in self.issues)),
                'files_with_warnings': len(set(warning['file'] for warning in self.warnings))
            }
        }
        
    def print_summary(self):
        """Print validation summary."""
        print("\n" + "=" * 60)
        print("📚 DOCUMENTATION VALIDATION SUMMARY")
        print("=" * 60)
        
        print(f"Files checked: {self.stats['files_checked']}")
        print(f"Examples validated: {self.stats['examples_validated']}")
        
        if self.issues:
            print(f"\n❌ Issues found: {len(self.issues)}")
            for issue in self.issues[:5]:  # Show first 5 issues
                print(f"   • {issue['type']}: {issue['message']}")
                print(f"     File: {Path(issue['file']).name}")
            if len(self.issues) > 5:
                print(f"   ... and {len(self.issues) - 5} more issues")
        else:
            print("\n✅ No critical issues found")
            
        if self.warnings:
            print(f"\n⚠️  Warnings: {len(self.warnings)}")
            for warning in self.warnings[:3]:  # Show first 3 warnings
                print(f"   • {warning['type']}: {warning['message']}")
            if len(self.warnings) > 3:
                print(f"   ... and {len(self.warnings) - 3} more warnings")
                
    def _get_timestamp(self) -> str:
        """Get current timestamp."""
        from datetime import datetime
        return datetime.now().isoformat()

def main():
    parser = argparse.ArgumentParser(description="Validate ECScope documentation")
    parser.add_argument("--source", type=Path, required=True, help="Source directory to validate")
    parser.add_argument("--examples", type=Path, help="Examples directory to validate")
    parser.add_argument("--output", type=Path, help="Output JSON report file")
    parser.add_argument("--fail-on-issues", action="store_true", help="Exit with error if issues found")
    
    args = parser.parse_args()
    
    # Validate arguments
    if not args.source.exists():
        print(f"❌ Source directory not found: {args.source}")
        sys.exit(1)
        
    # Run validation
    validator = DocumentationValidator()
    
    source_valid = validator.validate_source_documentation(args.source)
    
    examples_valid = True
    if args.examples:
        examples_valid = validator.validate_examples(args.examples)
    
    # Generate and save report
    report = validator.generate_report()
    
    if args.output:
        with open(args.output, 'w') as f:
            json.dump(report, f, indent=2)
        print(f"📝 Validation report saved to: {args.output}")
    
    # Print summary
    validator.print_summary()
    
    # Exit with appropriate code
    overall_valid = source_valid and examples_valid
    if args.fail_on_issues and not overall_valid:
        print(f"\n❌ Documentation validation failed")
        sys.exit(1)
    else:
        print(f"\n✅ Documentation validation completed")
        sys.exit(0)

if __name__ == "__main__":
    main()