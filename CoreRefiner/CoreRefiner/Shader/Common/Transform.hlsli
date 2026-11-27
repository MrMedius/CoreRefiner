cbuffer TransformCBuf : register(b0)
{
    // 变换矩阵。模型空间 -> 视图空间 -> 裁剪空间。
    matrix model;
    // 视图矩阵。视图空间 -> 裁剪空间。
    matrix modelView;
    // 投影矩阵。裁剪空间 -> 裁剪空间。
    matrix modelViewProj;
};