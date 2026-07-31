
#pragma once

namespace render
{
struct Color
{
	float R;
	float G;
	float B;
	float A;
};
class Curve
{


};

/**
 * > 每一个光源都有一个光源颜色和一个距离衰减曲线(distance falloff curve) 和一个方向衰
 * > 减函数(direction falloff curve), 所以其最终的颜色:
 * >     FinalColor = LightColor * DistanceDecayCurve(distance) * DirectionDecayCurve(direction)
 *
 * > 光源的类型往往分为多种: 
 * > 1. 点光源
 * > 2. 聚光灯
 * > 3. 平行光
 * > 4. 面光源
 * > 5. 环境光
 * > 6. 体积光
 * > 7. IES 光源(由照明协会提供的光源分布数据)
 * > 8. 类似火把的闪烁光源, 相当于点光源的一种
 */
class LightBase
{

public:
	LightBase() = default;
	virtual ~LightBase() = default;

	Curve getDistanceDecayCurve() noexcept;
	Curve getDirectionDecayCurve() noexcept;
	Color getLightColor() noexcept;

	LightBase& setLightColor(const Color& InLightColor) noexcept
	{
		this->LightColor = InLightColor;
		return *this;
	}
	LightBase& setDistanceDecayCurve(const Curve& InDistanceDecayCurve) noexcept
	{
		this->DistanceDecayCurve = InDistanceDecayCurve;
		return *this;
	}
	LightBase& setDirectionDecayCurve(const Curve& InDirectionDecayCurve) noexcept
	{
		this->DirectionDecayCurve = InDirectionDecayCurve;
		return *this;
	}
private:
	Curve DistanceDecayCurve;
	Curve DirectionDecayCurve;
	Color LightColor;
};

}