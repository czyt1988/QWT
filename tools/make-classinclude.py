#!/usr/bin/env python3
"""
扫描指定目录下的.h头文件，提取带有QWT_EXPORT导出符号的类和结构体
为每个导出的类/结构体生成对应的包含文件
同时处理固定文件和模板类
python make-classinclude.py --scan-dir ../src --output-dir ../classincludes
"""
import os
import re
import argparse
import sys
from pathlib import Path

# 固定文件映射
FIXED_FILES = {
    'QwtMath': 'qwt_math.h',
    'QwtGlobal': 'qwt_global.h'
}

def parse_arguments():
    """解析命令行参数"""
    parser = argparse.ArgumentParser(
        description='扫描头文件并生成QWT导出类的包含文件'
    )
    parser.add_argument(
        '--scan-dir', 
        required=True,
        help='要扫描的头文件目录'
    )
    parser.add_argument(
        '--output-dir', 
        required=True,
        help='输出文件目录'
    )
    return parser.parse_args()

def find_header_files(scan_dir):
    """查找目录下所有的.h文件"""
    header_files = []
    scan_path = Path(scan_dir)
    
    if not scan_path.exists():
        print(f"错误: 扫描目录不存在: {scan_dir}")
        return []
    
    for file_path in scan_path.rglob("*.h"):
        header_files.append(file_path)
    
    print(f"找到 {len(header_files)} 个头文件")
    return header_files

def extract_exported_classes(file_path):
    """
    从头文件中提取带有QWT_EXPORT的类和结构体，以及模板类
    返回类/结构体名称列表
    """
    exported_classes = []
    
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        # 正则表达式匹配QWT_EXPORT的类和结构体
        # 匹配模式: class QWT_EXPORT 类名 或 struct QWT_EXPORT 结构体名
        export_patterns = [
            r'class\s+QWT_EXPORT\s+(\w+)\s*[:{<]',  # class QWT_EXPORT ClassName
            r'struct\s+QWT_EXPORT\s+(\w+)\s*[:{<]', # struct QWT_EXPORT StructName
        ]
        
        for pattern in export_patterns:
            matches = re.findall(pattern, content)
            for class_name in matches:
                if class_name not in exported_classes:
                    exported_classes.append(class_name)
        
        # 匹配模板类
        # 匹配模式: template<...> class 类名 或 template<...> struct 结构体名
        template_patterns = [
            r'template\s*<[^>]*>\s*class\s+(\w+)\s*[:{<]',  # template<...> class ClassName
            r'template\s*<[^>]*>\s*struct\s+(\w+)\s*[:{<]', # template<...> struct StructName
        ]
        
        for pattern in template_patterns:
            matches = re.findall(pattern, content)
            for class_name in matches:
                # 排除一些常见的内部模板类
                if (class_name not in exported_classes and 
                    not class_name.startswith('QwtArray') and
                    not class_name.startswith('QwtVector') and
                    class_name not in ['QMap', 'QList', 'QVector', 'QSharedPointer']):
                    exported_classes.append(class_name)
        
    except Exception as e:
        print(f"读取文件 {file_path} 时出错: {e}")
    
    return exported_classes

def generate_include_file(output_dir, class_name, header_file_path=None, scan_dir=None, fixed_header=None):
    """
    为每个导出的类生成包含文件
    """
    try:
        # 创建输出文件路径
        output_file = Path(output_dir) / class_name
        
        if fixed_header:
            # 固定文件，直接使用指定的头文件
            include_content = f'#include "{fixed_header}"\n'
        else:
            # 计算头文件相对于扫描目录的相对路径
            rel_header_path = Path(header_file_path).relative_to(scan_dir)
            include_content = f'#include "{rel_header_path}"\n'
        
        # 写入包含指令
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(include_content)
        
        return True
        
    except Exception as e:
        print(f"生成文件 {class_name} 时出错: {e}")
        return False

def generate_fixed_files(output_dir, scan_dir):
    """
    生成固定文件
    """
    generated_count = 0
    
    for class_name, header_name in FIXED_FILES.items():
        # 检查头文件是否存在
        header_path = Path(scan_dir) / header_name
        if header_path.exists():
            if generate_include_file(output_dir, class_name, fixed_header=header_name):
                print(f"生成固定文件: {class_name} -> {header_name}")
                generated_count += 1
        else:
            print(f"警告: 固定文件所需的头文件不存在: {header_name}")
    
    return generated_count

def main():
    args = parse_arguments()
    
    # 检查并创建输出目录
    output_path = Path(args.output_dir)
    output_path.mkdir(parents=True, exist_ok=True)
    
    # 生成固定文件
    print("生成固定文件...")
    fixed_count = generate_fixed_files(args.output_dir, args.scan_dir)
    
    # 查找所有头文件
    header_files = find_header_files(args.scan_dir)
    if not header_files:
        print("没有找到头文件，程序退出")
        return
    
    total_classes = fixed_count
    processed_files = 0
    
    # 处理每个头文件
    for header_file in header_files:
        print(f"处理文件: {header_file}")
        
        # 提取导出的类和模板类
        exported_classes = extract_exported_classes(header_file)
        
        if exported_classes:
            print(f"  找到导出类和模板类: {', '.join(exported_classes)}")
            
            # 为每个导出的类生成文件
            for class_name in exported_classes:
                # 检查是否已经生成了固定文件，避免重复
                if class_name not in FIXED_FILES:
                    if generate_include_file(args.output_dir, class_name, header_file, args.scan_dir):
                        total_classes += 1
                        print(f"    生成文件: {class_name}")
        
        processed_files += 1
    
    print(f"\n处理完成!")
    print(f"扫描头文件: {processed_files} 个")
    print(f"生成包含文件: {total_classes} 个")
    print(f"输出目录: {args.output_dir}")

if __name__ == "__main__":
    main()