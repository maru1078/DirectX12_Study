#include "peraHeader.hlsli"

// ピクセルシェーダー
float4 ps(Output input) : SV_TARGET
{
	//return float4(input.uv, 1, 1);

	// 通常描画
	{
		//return tex.Sample(smp, input.uv);
    }

	float4 col = tex.Sample(smp, input.uv);

	// モノクロ化
    {
	    //float Y = dot(col.rgb, float3(0.299, 0.587, 0.114));

	    //return float4(Y, Y, Y, 1);
    }

	// 色の反転
	{
		//return float4(float3(1.0f, 1.0f, 1.0f) - col.rgb, col.a);
	}

	// レトロっぽく
	{
		//return float4(col.rgb - fmod(col.rgb, 0.25), col.a);
	}

	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);

	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	// 画素の平均化によるぼかし
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)); // 左上
		//ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)); // 上
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)); // 右上

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)); // 左
		//ret += tex.Sample(smp, input.uv); // 自分
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)); // 右

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)); // 左下
		//ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)); // 下
		//ret += tex.Sample(smp, input.uv + float2(2 * dy, 2 * dy)); // 右下
		//return ret / 9.0f;
	}

	//エンボス加工
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 2; // 左上
		//ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)); // 上
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0; // 右上

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)); // 左
		//ret += tex.Sample(smp, input.uv); // 自分
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1; // 右

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 0; // 左下
		//ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1; // 下
		//ret += tex.Sample(smp, input.uv + float2(2 * dy, 2 * dy)) * -2; // 右下

		//return ret;
	}

	// シャープ（エッジの強調）
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 0; // 左上
		//ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)) * -1; // 上
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0; // 右上

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1; // 左
		//ret += tex.Sample(smp, input.uv) * 5; // 自分
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1; // 右

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 0; // 左下
		//ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1; // 下
		//ret += tex.Sample(smp, input.uv + float2(2 * dy, 2 * dy)) * 0; // 右下

		//return ret;
	}

	// 輪郭線の抽出（色付き）
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 0; // 左上
		//ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)) * -1; // 上
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0; // 右上

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1; // 左
		//ret += tex.Sample(smp, input.uv) * 4; // 自分
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1; // 右

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 0; // 左下
		//ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1; // 下
		//ret += tex.Sample(smp, input.uv + float2(2 * dy, 2 * dy)) * 0; // 右下

		//return ret;
	}

	// 輪郭線の抽出（黒の線画）
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 0; // 左上
		//ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)) * -1; // 上
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0; // 右上

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1; // 左
		//ret += tex.Sample(smp, input.uv) * 4; // 自分
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1; // 右

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 0; // 左下
		//ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1; // 下
		//ret += tex.Sample(smp, input.uv + float2(2 * dy, 2 * dy)) * 0; // 右下

		//// モノクロにする
		//float Y = dot(ret.rgb, float3(0.299, 0.587, 0.114));
		//// 反転（線を黒にする）
		//Y = 1.0 - Y;
		//// 線の色を濃くする
		//Y = pow(Y, 10);
		//Y = step(0.2, Y);

		//return float4(Y, Y, Y, col.a);
	}

	// ガウシアンぼかし（簡単なほう）
	{
		//// 最上段
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 1 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 2 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, 2 * dy)) * 6 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, 2 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 2 * dy)) * 1 / 256;

		//// 1つ上段
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 1 * dy)) *  4 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, 1 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, 1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 1 * dy)) *  4 / 256;

		//// 中段
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0 * dy)) *  6 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 0 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, 0 * dy)) * 36 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, 0 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 0 * dy)) *  6 / 256;

		//// 1つ下段
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -1 * dy)) *  4 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, -1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, -1 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, -1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, -1 * dy)) *  4 / 256;

		//// 最下段
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 1 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, -2 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, -2 * dy)) * 6 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, -2 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, -2 * dy)) * 1 / 256;

		//return ret;
	}

	// ガウシアンぼかし（難しい方）
	{
		//// 最上段
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 1 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 2 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2(0 * dx, 2 * dy)) * 6 / 256;
		//ret += tex.Sample(smp, input.uv + float2(1 * dx, 2 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * 1 / 256;

		//// 1つ上段
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 1 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2(0 * dx, 1 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2(1 * dx, 1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 1 * dy)) * 4 / 256;

		//// 中段
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0 * dy)) * 6 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 0 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2(0 * dx, 0 * dy)) * 36 / 256;
		//ret += tex.Sample(smp, input.uv + float2(1 * dx, 0 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 0 * dy)) * 6 / 256;

		//// 1つ下段
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -1 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, -1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2(0 * dx, -1 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2(1 * dx, -1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, -1 * dy)) * 4 / 256;

		//// 最下段
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 1 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, -2 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2(0 * dx, -2 * dy)) * 6 / 256;
		//ret += tex.Sample(smp, input.uv + float2(1 * dx, -2 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 1 / 256;

		//ret += bkweights[0] * col;

		//for (int i = 1; i < 8; ++i)
		//{
		//	ret += bkweights[i >> 2][i % 4]
		//		* tex.Sample(smp, input.uv + float2(i * dx, 0));

		//	ret += bkweights[i >> 2][i % 4]
		//		* tex.Sample(smp, input.uv + float2(-i * dx, 0));
		//}

		//return float4(ret.rgb, col.a);
	}

	// シャドウマップ
	{
		//float dep = pow(depthTex.Sample(smp, input.uv), 20);
		//return float4(dep, dep, dep, 1);
	}

	// マルチレンダーターゲット
	{
		if (input.uv.x < 0.2 && input.uv.y < 0.2) // 深度出力
		{
			float depth = depthTex.Sample(smp, input.uv * 5);
			depth = pow(depth, 50);
			return float4(depth, depth, depth, 1);
		}
		else if (input.uv.x < 0.2 && input.uv.y < 0.4) // ライトからの深度出力
		{
			//return float4(1.0f, 0.0f, 0.0f, 1.0f);
			float depth = lightDepthTex.Sample(smp, (input.uv - float2(0, 0.2)) * 5);

			//depth = 1 - depth;
			return float4(depth, depth, depth, 1);
		}
		else if (input.uv.x < 0.2 && input.uv.y < 0.6) // 法線出力
		{
			//return float4(1.0f, 1.0f, 0.0f, 1.0f);
			return texNormal.Sample(smp, (input.uv - float2(0, 0.4)) * 5);
		}
	}

	return tex.Sample(smp, input.uv);

	// ディファードシェーディング
	{
		//float4 normal = texNormal.Sample(smp, input.uv);
        //normal = normal * 2.0f - 1.0f;
        
        //float3 light = normalize(float3(1.0f, -1.0f, 1.0f));
        //const float ambient = 0.25f;
        //float diffB = max(saturate(dot(normal.xyz, -light)), ambient);
        
        //return tex.Sample(smp, input.uv) * float4(diffB, diffB, diffB, 1);
	}
}