#ifndef QWT_ALGORITHM_HPP
#define QWT_ALGORITHM_HPP
#include <algorithm>  // std::find
#include <iterator>   // std::prev

/**
 * @brief 从迭代器范围中查找下一个元素的迭代器（支持循环切换）
 *
 * 该函数在给定的迭代器范围 [begin, end) 中，根据当前元素和方向（向前/向后）查找下一个元素的迭代器。
 * 若当前元素不在范围内，默认从第一个元素开始计算；若范围为空，返回 end 迭代器。
 * 支持双向迭代器（如 QList、std::vector、std::list 等容器的迭代器），适用于循环切换场景（如轮询选择元素）。
 *
 * @tparam Iter 迭代器类型，需满足双向迭代器要求（支持 ++、--、* 操作及相等比较）
 * @param begin 范围的起始迭代器（包含）
 * @param end   范围的结束迭代器（不包含）
 * @param current 当前元素，函数将从该元素开始计算下一个元素
 * @param forward 方向标志：true 表示向前（++），false 表示向后（--）
 * @return Iter 下一个元素的迭代器；若范围为空，返回 end
 *
 * @note 1. 若 current 不在 [begin, end) 范围内，默认从第一个元素（begin）开始计算
 *       2. 当向前切换到 end 时，自动循环到 begin；当向后切换到 begin 时，自动循环到 end 的前一个元素
 *       3. 要求元素类型支持 == 比较（用于 std::find 查找 current）
 *
 * 示例1：使用 std::vector 循环切换
 * @code
 * #include <vector>
 * #include <iostream>
 *
 * int main() {
 *     std::vector<int> nums = {10, 20, 30, 40};
 *     int current = 20;
 *
 *     // 向前切换：20 → 30
 *     auto it = qwtSelectNextIterator(nums.begin(), nums.end(), current, true);
 *     std::cout << *it; // 输出：30
 *
 *     // 向后切换：20 → 10
 *     it = qwtSelectNextIterator(nums.begin(), nums.end(), current, false);
 *     std::cout << *it; // 输出：10
 *     return 0;
 * }
 * @endcode
 *
 * 示例2：循环到首尾边界
 * @code
 * #include <QList>
 * #include <QDebug>
 *
 * int main() {
 *     QList<QString> strs = {"A", "B", "C"};
 *     QString current = "C";
 *
 *     // 向前切换：C → A（到达 end 后循环到 begin）
 *     auto it = qwtSelectNextIterator(strs.begin(), strs.end(), current, true);
 *     qDebug() << *it; // 输出："A"
 *
 *     current = "A";
 *     // 向后切换：A → C（到达 begin 后循环到 end 前一个）
 *     it = qwtSelectNextIterator(strs.begin(), strs.end(), current, false);
 *     qDebug() << *it; // 输出："C"
 *     return 0;
 * }
 * @endcode
 *
 * 示例3：当前元素不在范围内
 * @code
 * #include <std::list>
 * #include <iostream>
 *
 * int main() {
 *     std::list<double> vals = {1.5, 2.5, 3.5};
 *     double current = 99.9; // 不在范围内
 *
 *     // 默认从第一个元素（1.5）开始向前切换 → 2.5
 *     auto it = qwtSelectNextIterator(vals.begin(), vals.end(), current, true);
 *     std::cout << *it; // 输出：2.5
 *     return 0;
 * }
 * @endcode
 *
 * 示例4：空范围处理
 * @code
 * #include <vector>
 * #include <iostream>
 *
 * int main() {
 *     std::vector<char> empty;
 *     char current = 'a';
 *
 *     // 空范围返回 end
 *     auto it = qwtSelectNextIterator(empty.begin(), empty.end(), current, true);
 *     if (it == empty.end()) {
 *         std::cout << "范围为空"; // 输出：范围为空
 *     }
 *     return 0;
 * }
 * @endcode
 */
template< typename Iter >
Iter qwtSelectNextIterator(Iter begin, Iter end, typename std::iterator_traits< Iter >::value_type current, bool forward)
{
    // 空范围直接返回 end
    if (begin == end) {
        return end;
    }

    // 查找当前元素在范围内的位置
    Iter ite = std::find(begin, end, current);

    // 若当前元素不在范围内，默认从第一个元素开始
    if (ite == end) {
        ite = begin;
    }

    // 根据方向计算下一个迭代器
    if (forward) {
        ++ite;
        if (ite == end) {  // 到达末尾，循环到开头
            ite = begin;
        }
    } else {
        if (ite != begin) {  // 未到开头，向前移动
            --ite;
        } else {  // 已在开头，循环到末尾
            ite = std::prev(end);
        }
    }

    return ite;
}

#endif  // QWT_ALGORITHM_HPP
