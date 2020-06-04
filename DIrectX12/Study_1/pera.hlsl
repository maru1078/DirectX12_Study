#include "peraHeader.hlsli"

float4 VerticalBokehPS(Output input) : SV_TARGET
{
	{
	    //float w, h, level;
	    //tex.GetDimensions(0, w, h, level);
	    //
	    //float dy = 1.0f / h;
	    //float4 ret = float4(0, 0, 0, 0);
	    //float4 col = tex.Sample(smp, input.uv);
	    //
	    //ret += bkweights[0] * col;
	    //
	    //for (int i = 1; i < 8; ++i)
	    //{
	    //	ret += bkweights[i >> 2][i % 4]
	    //		* tex.Sample(smp, input.uv + float2(0, dy * i));
	    //
	    //	ret += bkweights[i >> 2][i % 4]
	    //		* tex.Sample(smp, input.uv + float2(0, dy * -i));
	    //}
	    //
	    //return float4(ret.rgb, col.a);
	}

	// ポストエフェクト（歪み）
    {
	    float2 nmTex = effectTex.Sample(smp, input.uv).xy;
	    nmTex = nmTex * 2.0f - 1.0f;
	    
	    // nmTexの範囲は -1～1 だが、幅 1 がテクスチャ1枚の
	    // 大きさであり、-1～1 では歪みすぎるため 0.1 を乗算している
	    return tex.Sample(smp, input.uv + nmTex * 0.1f);
    }
}