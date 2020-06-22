#include "PeraHeader.hlsli"

// �K�E�V�A���ڂ����c�p
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

// �m�[�}���}�b�v�ɂ��ڂ����p
float4 EffectPS(Output input) : SV_TARGET
{
	float2 nmTex = effectTex.Sample(smp, input.uv).xy;
	nmTex = nmTex * 2.0 - 1.0;

	// nmTex�͈̔͂�-1�`1�����A��1���e�N�X�`��1���̑傫���ł���A
	// -1�`1�ł͘c�݉߂��邽��0.1����Z
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
		if (input.uv.y < 0.2) // �[�x�o��
		{
			float depth = depthTex.Sample(smp, input.uv * 5);
			depth = 1.0 - pow(depth, 30);
			return float4(depth, depth, depth, 1.0);
		}
		else if (input.uv.y < 0.4) // ���C�g����̐[�x�o��
		{
			float depth = lightDepthTex.Sample(smp, (input.uv - float2(0.0, 0.2)) * 5);
			depth = 1.0 - depth;
			return float4(depth, depth, depth, 1.0);
		}
		else if (input.uv.y < 0.6) // �@���o��
		{
			return texNormal.Sample(smp, (input.uv - float2(0.0, 0.4)) * 5);
		}
	}

	// �[�x�\��
	{
		//float dep = pow(depthTex.Sample(smp, input.uv), 20);
		//return float4(dep, dep, dep, 1.0);
	}

	// �K�E�V�A���ڂ����i�{�i�Łj
	{
		//ret += bkweights[0] * col;

		//for (int i = 1; i < 8; ++i)
		//{
		//	ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(i * dx, 0));
		//	ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(-i * dx, 0));
		//}

		//return float4(ret.rgb, col.a);
	}

	// �K�E�V�A���ڂ����i�ȈՔŁj
	{
		//// ���̃s�N�Z���𒆐S�ɏc��5���ɂȂ�悤�ɉ��Z����
		//// �ŏ�i
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 1;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 2 * dy)) * 4;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, 2 * dy)) * 6;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, 2 * dy)) * 4;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 2 * dy)) * 1;

		//// 1��̒i
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 1 * dy)) *  4;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 1 * dy)) * 16;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, 1 * dy)) * 24;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, 1 * dy)) * 16;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 1 * dy)) *  4;

		//// ���i
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0 * dy)) *  6;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 0 * dy)) * 24;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, 0 * dy)) * 36;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, 0 * dy)) * 24;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 0 * dy)) *  6;

		//// 1���̒i
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -1 * dy)) *  4;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, -1 * dy)) * 16;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, -1 * dy)) * 24;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, -1 * dy)) * 16;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, -1 * dy)) *  4;

		//// �ŉ��i
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 1;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, -2 * dy)) * 4;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, -2 * dy)) * 6;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, -2 * dy)) * 4;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, -2 * dy)) * 1;

		//return ret / 256;
	}

	// �֊s�����o
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) *  0; // ����
		//ret += tex.Sample(smp, input.uv + float2(      0, -2 * dy)) * -1; // ��
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, -2 * dy)) *  0; // �E��

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1; // ��
		//ret += tex.Sample(smp, input.uv)                      *  4; // ����
		//ret += tex.Sample(smp, input.uv + float2 (2 * dx, 0)) * -1; // �E

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) *  0; // ����
		//ret += tex.Sample(smp, input.uv + float2(      0, 2 * dy)) * -1; // ��
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 2 * dy)) *  0; // �E��

		//// �F���]
		//float y = dot(ret.rgb, float3(0.299, 0.587, 0.114));

		//y = pow(1.0 - y, 10);

		//y = step(0.2, y); // ���������łĂ���Ƃ�����Ȃ����Ă�̂��ȁH

		//return float4(y, y, y, 1.0);
	}

	// �V���[�v�l�X�i�G�b�W�̋����j
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) *  0; // ����
		//ret += tex.Sample(smp, input.uv + float2(      0, -2 * dy)) * -1; // ��
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, -2 * dy)) *  0; // �E��

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1; // ��
		//ret += tex.Sample(smp, input.uv)                      *  5; // ����
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 0)) * -1; // �E

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) *  0; // ����
		//ret += tex.Sample(smp, input.uv + float2(      0, 2 * dy)) * -1; // ��
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 2 * dy)) *  0; // �E��

		//return ret;
	}

	// �G���{�X+���m�N��
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 2; // ����
	 //   ret += tex.Sample(smp, input.uv + float2(      0, -2 * dy)) * 1; // ��
	 //   ret += tex.Sample(smp, input.uv + float2( 2 * dx, -2 * dy)) * 0; // �E��
	 //   
	 //   ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) *  1; // ��
	 //   ret += tex.Sample(smp, input.uv)                      *  1; // ����
	 //   ret += tex.Sample(smp, input.uv + float2( 2 * dx, 0)) * -1; // �E
	 //   
	 //   ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) *  0; // ����
	 //   ret += tex.Sample(smp, input.uv + float2(      0, 2 * dy)) * -1; // ��
	 //   ret += tex.Sample(smp, input.uv + float2( 2 * dx, 2 * dy)) * -2; // �E��
	 //
		//float y = dot(ret.rgb, float3(0.299, 0.587, 0.114)); // ���m�N����
		//return float4(y, y, y, 1.0);
	}

	// �ڂ���
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)); // ����
		//ret += tex.Sample(smp, input.uv + float2(      0, -2 * dy)); // ��
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, -2 * dy)); // �E��

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)); // ��
		//ret += tex.Sample(smp, input.uv);                      // ����
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 0)); // �E

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)); // ����
		//ret += tex.Sample(smp, input.uv + float2(      0, 2 * dy)); // ��
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 2 * dy)); // �E��

		//return ret / 9.0; // ���ϒl�����
	}

	// ���g�����ۂ�
	{
		//return float4(col.rgb - fmod(col.rgb, 0.25), col.a);
	}

	// �F���]
	{
		//return float4(1.0 - col.rgb, col.a);
	}

	// ���m�N��
	{
		//float y = dot(col.rgb, float3(0.299, 0.587, 0.114));
		//return float4(y, y, y, 1.0);
	}

	// �ʏ�`��
	{
		return col;
	}
}