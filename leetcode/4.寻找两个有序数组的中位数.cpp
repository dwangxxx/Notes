/*
 * @lc app=leetcode.cn id=4 lang=cpp
 *
 * [4] 寻找两个有序数组的中位数
 */

// @lc code=start
class Solution{
public:
    double findMedianSortedArrays(vector<int>& nums1, vector<int>& nums2)
    {
        int len1 = nums1.size();
        int len2 = nums2.size();
        // 求第一个数的位置
        int k1 = (len1 + len2 + 1) / 2;
        // 求第二个数的位置
        int k2 = (len1 + len2 + 2) / 2;

        return (get_median(0, len1 - 1, nums1, 0, len2 - 1, nums2, k1) + get_median(0, len1 - 1, nums1, 0, len2 - 1, nums2, k2)) / 2;
    }

    double get_median(int start1, int end1, vector<int>& nums1, int start2, int end2, vector<int>& nums2, int k)
    {
        // 如果nums2的长度小于nums1,则将其调换位置,这样保证只有nums1会为空
        if(end1 - start1 > end2 - start2)
            return get_median(start2, end2, nums2, start1, end1, nums1, k);
        // 如果nums1为空,则直接返回nums2的第k个数
        if(start1 > end1)   return nums2[start2 + k - 1];
        // k = 1的情况单独处理
        if(k == 1)  return min(nums1[start1], nums2[start2]);
        int index1 = min(end1, start1 + k / 2 - 1);
        int index2 = min(end2, start2 + k / 2 - 1);
        // 如果nums1[index1] < nums2[index2],则将nums1的前几个数删除
        if(nums1[index1] < nums2[index2])
            return get_median(index1 + 1, end1, nums1, start2, end2, nums2, k - index1 + start1 - 1);
        else
            return get_median(start1, end1, nums1, index2 + 1, end2, nums2, k - index2 + start2 - 1);
    }
};
// @lc code = end
