#include "PeraHeader.hlsli"

float4 PeraPS(Output input) : SV_TARGET
{
	float4 col = tex.Sample(smp, input.uv);
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0 / w;
	float dy = 1.0 / h;
	float4 ret = float4(0, 0, 0, 0);

	// 輪郭線抽出
	{
		ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) *  0; // 左上
		ret += tex.Sample(smp, input.uv + float2(      0, -2 * dy)) * -1; // 上
		ret += tex.Sample(smp, input.uv + float2( 2 * dx, -2 * dy)) *  0; // 右上

		ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1; // 左
		ret += tex.Sample(smp, input.uv)                      *  4; // 自分
		ret += tex.Sample(smp, input.uv + float2 (2 * dx, 0)) * -1; // 右

		ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) *  0; // 左下
		ret += tex.Sample(smp, input.uv + float2(      0, 2 * dy)) * -1; // 下
		ret += tex.Sample(smp, input.uv + float2( 2 * dx, 2 * dy)) *  0; // 右下

		// 色反転
		float y = dot(ret.rgb, float3(0.299, 0.587, 0.114));

		y = pow(1.0 - y, 10);

		y = step(0.2, y); // 薄く黒がでているところをなくしてるのかな？

		return float4(y, y, y, 1.0);
	}

	// シャープネス（エッジの協調）
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) *  0; // 左上
		//ret += tex.Sample(smp, input.uv + float2(      0, -2 * dy)) * -1; // 上
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, -2 * dy)) *  0; // 右上

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1; // 左
		//ret += tex.Sample(smp, input.uv)                      *  5; // 自分
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 0)) * -1; // 右

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) *  0; // 左下
		//ret += tex.Sample(smp, input.uv + float2(      0, 2 * dy)) * -1; // 下
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 2 * dy)) *  0; // 右下

		//return ret;
	}

	// エンボス+モノクロ
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 2; // 左上
	 //   ret += tex.Sample(smp, input.uv + float2(      0, -2 * dy)) * 1; // 上
	 //   ret += tex.Sample(smp, input.uv + float2( 2 * dx, -2 * dy)) * 0; // 右上
	 //   
	 //   ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) *  1; // 左
	 //   ret += tex.Sample(smp, input.uv)                      *  1; // 自分
	 //   ret += tex.Sample(smp, input.uv + float2( 2 * dx, 0)) * -1; // 右
	 //   
	 //   ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) *  0; // 左下
	 //   ret += tex.Sample(smp, input.uv + float2(      0, 2 * dy)) * -1; // 下
	 //   ret += tex.Sample(smp, input.uv + float2( 2 * dx, 2 * dy)) * -2; // 右下
	 //
		//float y = dot(ret.rgb, float3(0.299, 0.587, 0.114)); // モノクロ化
		//return float4(y, y, y, 1.0);
	}

	// ぼかし
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)); // 左上
		//ret += tex.Sample(smp, input.uv + float2(      0, -2 * dy)); // 上
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, -2 * dy)); // 右上

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)); // 左
		//ret += tex.Sample(smp, input.uv);                      // 自分
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 0)); // 右

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)); // 左下
		//ret += tex.Sample(smp, input.uv + float2(      0, 2 * dy)); // 下
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 2 * dy)); // 右下

		//return ret / 9.0; // 平均値を取る
	}

	// レトロっぽく
	{
		//return float4(col.rgb - fmod(col.rgb, 0.25), col.a);
	}

	// 色反転
	{
		//return float4(1.0 - col.rgb, col.a);
	}

	// モノクロ
	{
		//float y = dot(col.rgb, float3(0.299, 0.587, 0.114));
		//return float4(y, y, y, 1.0);
	}

	// 通常描画
	{
		//return col;
	}
}