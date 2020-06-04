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

	// �|�X�g�G�t�F�N�g�i�c�݁j
    {
	    float2 nmTex = effectTex.Sample(smp, input.uv).xy;
	    nmTex = nmTex * 2.0f - 1.0f;
	    
	    // nmTex�͈̔͂� -1�`1 �����A�� 1 ���e�N�X�`��1����
	    // �傫���ł���A-1�`1 �ł͘c�݂����邽�� 0.1 ����Z���Ă���
	    return tex.Sample(smp, input.uv + nmTex * 0.1f);
    }
}