#include "PeraHeader.hlsli"

// ガウシアンぼかし縦用
float4 VerticalBokehPS(Output input) : SV_TARGET
{
	float w, h, level;
    tex.GetDimensions(0, w, h, level);
	
	float dy = 1.0 / h;
	float4 ret = float4(0, 0, 0, 0);
	float4 col = tex.Sample(smp, input.uv);

	ret += bkweights[0] * col;

	for (int i = 1; i < 8; ++i)
	{
		ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(0, i * dy));
		ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(0, -i * dy));
	}

	return float4(ret.rgb, col.a);
}

// ノーマルマップによるぼかし用
float4 EffectPS(Output input) : SV_TARGET
{
	float2 nmTex = effectTex.Sample(smp, input.uv).xy;
	nmTex = nmTex * 2.0 - 1.0;

	// nmTexの範囲は-1～1だが、幅1がテクスチャ1枚の大きさであり、
	// -1～1では歪み過ぎるため0.1を乗算
	return tex.Sample(smp, input.uv + nmTex * 0.1);
}

float4 PeraPS(Output input) : SV_TARGET
{
	float4 col = tex.Sample(smp, input.uv);
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0 / w;
	float dy = 1.0 / h;
	float4 ret = float4(0, 0, 0, 0);

	if (input.uv.x < 0.2)
	{
		if (input.uv.y < 0.2) // 深度出力
		{
			float depth = depthTex.Sample(smp, input.uv * 5);
			depth = 1.0 - pow(depth, 30);
			return float4(depth, depth, depth, 1.0);
		}
		else if (input.uv.y < 0.4) // ライトからの深度出力
		{
			float depth = lightDepthTex.Sample(smp, (input.uv - float2(0.0, 0.2)) * 5);
			depth = 1.0 - depth;
			return float4(depth, depth, depth, 1.0);
		}
		else if (input.uv.y < 0.6) // 法線出力
		{
			return texNormal.Sample(smp, (input.uv - float2(0.0, 0.4)) * 5);
		}
	}

	// 深度表示
	{
		//float dep = pow(depthTex.Sample(smp, input.uv), 20);
		//return float4(dep, dep, dep, 1.0);
	}

	// ガウシアンぼかし（本格版）
	{
		//ret += bkweights[0] * col;

		//for (int i = 1; i < 8; ++i)
		//{
		//	ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(i * dx, 0));
		//	ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(-i * dx, 0));
		//}

		//return float4(ret.rgb, col.a);
	}

	// ガウシアンぼかし（簡易版）
	{
		//// 今のピクセルを中心に縦横5ずつになるように加算する
		//// 最上段
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 1;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 2 * dy)) * 4;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, 2 * dy)) * 6;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, 2 * dy)) * 4;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 2 * dy)) * 1;

		//// 1つ上の段
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 1 * dy)) *  4;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 1 * dy)) * 16;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, 1 * dy)) * 24;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, 1 * dy)) * 16;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 1 * dy)) *  4;

		//// 中段
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0 * dy)) *  6;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 0 * dy)) * 24;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, 0 * dy)) * 36;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, 0 * dy)) * 24;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 0 * dy)) *  6;

		//// 1つ下の段
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -1 * dy)) *  4;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, -1 * dy)) * 16;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, -1 * dy)) * 24;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, -1 * dy)) * 16;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, -1 * dy)) *  4;

		//// 最下段
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 1;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, -2 * dy)) * 4;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, -2 * dy)) * 6;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, -2 * dy)) * 4;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, -2 * dy)) * 1;

		//return ret / 256;
	}

	// 輪郭線抽出
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) *  0; // 左上
		//ret += tex.Sample(smp, input.uv + float2(      0, -2 * dy)) * -1; // 上
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, -2 * dy)) *  0; // 右上

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1; // 左
		//ret += tex.Sample(smp, input.uv)                      *  4; // 自分
		//ret += tex.Sample(smp, input.uv + float2 (2 * dx, 0)) * -1; // 右

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) *  0; // 左下
		//ret += tex.Sample(smp, input.uv + float2(      0, 2 * dy)) * -1; // 下
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 2 * dy)) *  0; // 右下

		//// 色反転
		//float y = dot(ret.rgb, float3(0.299, 0.587, 0.114));

		//y = pow(1.0 - y, 10);

		//y = step(0.2, y); // 薄く黒がでているところをなくしてるのかな？

		//return float4(y, y, y, 1.0);
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
		return col;
	}
}