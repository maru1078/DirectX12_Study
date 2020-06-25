#include "PeraHeader.hlsli"

float4 Get5x5GaussianBlur(Texture2D<float4> tex, SamplerState smp, float2 uv, float dx, float dy, float4 rect)
{
	float4 ret = tex.Sample(smp, uv);

	float l1 = -dx, l2 = -2 * dx;
	float r1 =  dx, r2 =  2 * dx;
	float u1 = -dy, u2 = -2 * dy;
	float d1 =  dy, d2 =  2 * dy;

	// left?
	l1 = max(uv.x + l1, rect.x) - uv.x;
	l2 = max(uv.x + l2, rect.x) - uv.x;

	// right?
	r1 = min(uv.x + r1, rect.z - dx) - uv.x;
	r2 = min(uv.x + r2, rect.z - dx) - uv.x;

	// up?
	u1 = max(uv.y + u1, rect.y) - uv.y;
	u2 = max(uv.y + u2, rect.y) - uv.y;

	// down?
	d1 = min(uv.y + d1, rect.w - dy) - uv.y;
	d2 = min(uv.y + d2, rect.w - dy) - uv.y;

	// 最上段
	ret += tex.Sample(smp, uv + float2(l2, u2)) * 1;
	ret += tex.Sample(smp, uv + float2(l1, u2)) * 4;
	ret += tex.Sample(smp, uv + float2( 0, u2)) * 6;
	ret += tex.Sample(smp, uv + float2(r1, u2)) * 4;
	ret += tex.Sample(smp, uv + float2(r2, u2)) * 1;

	// 1つ上の段
	ret += tex.Sample(smp, uv + float2(l2, u1)) *  4;
	ret += tex.Sample(smp, uv + float2(l1, u1)) * 16;
	ret += tex.Sample(smp, uv + float2( 0, u1)) * 24;
	ret += tex.Sample(smp, uv + float2(r1, u1)) * 16;
	ret += tex.Sample(smp, uv + float2(r2, u1)) *  4;

	// 中段
	ret += tex.Sample(smp, uv + float2(l2, 0)) *  6;
	ret += tex.Sample(smp, uv + float2(l1, 0)) * 24;
	ret += tex.Sample(smp, uv + float2( 0, 0)) * 36;
	ret += tex.Sample(smp, uv + float2(r1, 0)) * 24;
	ret += tex.Sample(smp, uv + float2(r2, 0)) *  6;

	// 1つ下の段
	ret += tex.Sample(smp, uv + float2(l2, d1)) *  4;
	ret += tex.Sample(smp, uv + float2(l1, d1)) * 16;
	ret += tex.Sample(smp, uv + float2( 0, d1)) * 24;
	ret += tex.Sample(smp, uv + float2(r1, d1)) * 16;
	ret += tex.Sample(smp, uv + float2(r2, d1)) *  4;

	// 最下段
	ret += tex.Sample(smp, uv + float2(l2, d2)) * 1;
	ret += tex.Sample(smp, uv + float2(l1, d2)) * 4;
	ret += tex.Sample(smp, uv + float2( 0, d2)) * 6;
	ret += tex.Sample(smp, uv + float2(r1, d2)) * 4;
	ret += tex.Sample(smp, uv + float2(r2, d2)) * 1;

	return float4(ret.rgb / 256, ret.a);
}

// 現在のuv値をもとに乱数を返す
float random(float2 uv)
{
	return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

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
	// エフェクトあり
	float2 nmTex = effectTex.Sample(smp, input.uv).xy;
	nmTex = nmTex * 2.0 - 1.0;

	// nmTexの範囲は-1～1だが、幅1がテクスチャ1枚の大きさであり、
	// -1～1では歪み過ぎるため0.1を乗算
	return tex.Sample(smp, input.uv + nmTex * 0.1);
}

//メインテクスチャを5x5ガウスでぼかす
BlurOutput BlurPS(Output input) : SV_TARGET
{
	float w, h, miplevels;
	tex.GetDimensions(0, w, h, miplevels);
	float dx = 1.0 / w;
	float dy = 1.0 / h;

	BlurOutput ret;
	ret.highLum = Get5x5GaussianBlur(texHighLum, smp, input.uv, dx, dy, float4(0, 0, 1, 1));
	ret.col = Get5x5GaussianBlur(tex, smp, input.uv, dx, dy, float4(0, 0, 1, 1));
	return ret;
}

// SSAO（乗算用の明度のみ情報を返せればよい）
float SsaoPs(Output input) : SV_TARGET
{
	float dp = depthTex.Sample(smp, input.uv); // 現在のuvの深度

    float w, h, miplevels;
	depthTex.GetDimensions(0, w, h, miplevels);

	float dx = 1.0 / w;
	float dy = 1.0 / h;

	// SSAO
	// 元の座標を復元する
	float4 repos = mul(invproj, float4(input.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), dp, 1));
	repos.xyz /= repos.w;

	float div = 0.0;
	float ao = 0.0;
	float3 norm = normalize((texNormal.Sample(smp, input.uv).xyz * 2) - 1);
	const int trycnt = 256;
	const float radius = 0.5;

	if (dp < 1.0)
	{
		for (int i = 0; i < trycnt; ++i)
		{
			float rnd1 = random(float2(i * dx, i * dy)) * 2 - 1;
			float rnd2 = random(float2(rnd1, i * dy)) * 2 - 1;
			float rnd3 = random(float2(rnd2, rnd1)) * 2 - 1;
			float3 omega = normalize(float3(rnd1, rnd2, rnd3));

			// 乱数の結果法線の反対側を向いていたら反転する
			float dt = dot(norm, omega);
			float sgn = sign(dt);
			omega *= sign(dt);

			// 結果の座標を再び射影変換する
			float4 rpos = mul(proj, float4(repos.xyz + omega * radius, 1));
			rpos.xyz /= rpos.w;
			dt *= sgn; // 正の値にしてcosθを得る
			div += dt; // 射影を考えない結果を加算する

			// 計算結果が現在の場所の深度より奥に入っているなら
			// 遮蔽されているので加算
			ao += step(depthTex.Sample(smp, (rpos.xy + float2(1, -1))
				* float2(0.5, -0.5)), rpos.z) * dt;
		}

		ao /= div;
	}

	return 1.0 - ao;
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
		else if (input.uv.y < 0.8)
		{
			return texHighLum.Sample(smp, (input.uv - float2(0.0, 0.6)) * 5);
		}
		else if (input.uv.y < 1.0)
		{
			float s = SSAOTex.Sample(smp, (input.uv - float2(0.0, 0.8)) * 5);
			return float4(s, s, s, 1.0);
		}
	}

	// SSAOあり
	float s = SSAOTex.Sample(smp, input.uv);
	return float4(col.rgb * s, col.a);

	float4 bloomAccum = float4(0.0, 0.0, 0.0, 0.0);
	float2 uvSize = float2(1.0, 0.5);
	float2 uvOfst = float2(0.0, 0.0);

	for (int i = 0; i < 8; ++i)
	{
		bloomAccum += Get5x5GaussianBlur(texShrinkHighLum, smp, 
			input.uv * uvSize + uvOfst, dx, dy, float4(uvOfst, uvOfst + uvSize));
		uvOfst.y += uvSize.y;
		uvSize *= 0.5;
	}

	// 画面真ん中からの深度の差を図る
	float depthDiff = abs(depthTex.Sample(smp, float2(0.5, 0.5))
		- depthTex.Sample(smp, input.uv));

	depthDiff = pow(depthDiff, 0.5);
	uvSize = float2(1.0, 0.5);
	uvOfst = float2(0.0, 0.0);

	float t = depthDiff * 8;
	float no;
	t = modf(t, no);

	float4 retColor[2];
	retColor[0] = tex.Sample(smp, input.uv); // 通常テクスチャ
	if (no == 0.0)
	{
		retColor[1] = Get5x5GaussianBlur(
			texShrink, smp, input.uv * uvSize + uvOfst,
			dx, dy, float4(uvOfst, uvOfst + uvSize));
	}
	else
	{
		for (int i = 1; i <= 8; ++i)
		{
			if (i - no < 0)
			{
				continue;
			}

			retColor[i - no] = Get5x5GaussianBlur(
				texShrink, smp, input.uv * uvSize + uvOfst,
				dx, dy, float4(uvOfst, uvOfst + uvSize));

			uvOfst.y += uvSize.y;
			uvSize *= 0.5;
			if (i - no > 1)
			{
				break;
			}
		}
	}
	// 深度に対応したぼかし
	return lerp(retColor[0], retColor[1], t);

	// 高輝度の部分を光らせる
	return tex.Sample(smp, input.uv) // 通常テクスチャ
		+ Get5x5GaussianBlur(texHighLum, smp, input.uv, dx, dy, float4(0, 0, 1, 1))
		+ saturate(bloomAccum);

	// ディファードシェーディング用の処理
	{
		//float4 normal = texNormal.Sample(smp, input.uv);
		//normal = normal * 2.0 - 1.0;

		//float3 light = normalize(float3(1.0, -1.0, 1.0));
		//const float ambient = 0.25;
		//float diffB = max(saturate(dot(normal.xyz, -light)), ambient);
		//return tex.Sample(smp, input.uv) * float4(diffB, diffB, diffB, 1.0);
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

		//return (ret / 256);
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