#include "PeraHeader.hlsli"

float4 Get5x5GaussianBlur(Texture2D<float4> tex, SamplerState smp, float2 uv, float dx, float dy)
{
	float4 ret = float4(0.0, 0.0, 0.0, 0.0);

	// �ŏ�i
	ret += tex.Sample(smp, uv + float2(-2 * dx, 2 * dy)) * 1;
	ret += tex.Sample(smp, uv + float2(-1 * dx, 2 * dy)) * 4;
	ret += tex.Sample(smp, uv + float2(0 * dx, 2 * dy)) * 6;
	ret += tex.Sample(smp, uv + float2(1 * dx, 2 * dy)) * 4;
	ret += tex.Sample(smp, uv + float2(2 * dx, 2 * dy)) * 1;

	// 1��̒i
	ret += tex.Sample(smp, uv + float2(-2 * dx, 1 * dy)) * 4;
	ret += tex.Sample(smp, uv + float2(-1 * dx, 1 * dy)) * 16;
	ret += tex.Sample(smp, uv + float2(0 * dx, 1 * dy)) * 24;
	ret += tex.Sample(smp, uv + float2(1 * dx, 1 * dy)) * 16;
	ret += tex.Sample(smp, uv + float2(2 * dx, 1 * dy)) * 4;

	// ���i
	ret += tex.Sample(smp, uv + float2(-2 * dx, 0 * dy)) * 6;
	ret += tex.Sample(smp, uv + float2(-1 * dx, 0 * dy)) * 24;
	ret += tex.Sample(smp, uv + float2(0 * dx, 0 * dy)) * 36;
	ret += tex.Sample(smp, uv + float2(1 * dx, 0 * dy)) * 24;
	ret += tex.Sample(smp, uv + float2(2 * dx, 0 * dy)) * 6;

	// 1���̒i
	ret += tex.Sample(smp, uv + float2(-2 * dx, -1 * dy)) * 4;
	ret += tex.Sample(smp, uv + float2(-1 * dx, -1 * dy)) * 16;
	ret += tex.Sample(smp, uv + float2(0 * dx, -1 * dy)) * 24;
	ret += tex.Sample(smp, uv + float2(1 * dx, -1 * dy)) * 16;
	ret += tex.Sample(smp, uv + float2(2 * dx, -1 * dy)) * 4;

	// �ŉ��i
	ret += tex.Sample(smp, uv + float2(-2 * dx, -2 * dy)) * 1;
	ret += tex.Sample(smp, uv + float2(-1 * dx, -2 * dy)) * 4;
	ret += tex.Sample(smp, uv + float2(0 * dx, -2 * dy)) * 6;
	ret += tex.Sample(smp, uv + float2(1 * dx, -2 * dy)) * 4;
	ret += tex.Sample(smp, uv + float2(2 * dx, -2 * dy)) * 1;

	return (ret / 256);
}

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
	// �ʏ�`��
	return tex.Sample(smp, input.uv);


	float2 nmTex = effectTex.Sample(smp, input.uv).xy;
	nmTex = nmTex * 2.0 - 1.0;

	// nmTex�͈̔͂�-1�`1�����A��1���e�N�X�`��1���̑傫���ł���A
	// -1�`1�ł͘c�݉߂��邽��0.1����Z
	return tex.Sample(smp, input.uv + nmTex * 0.1);
}

// ���C���e�N�X�`����5x5�K�E�X�łڂ���
float4 BlurPS(Output input) : SV_TARGET
{
	float w, h, miplevels;
    tex.GetDimensions(0, w, h, miplevels);
	return Get5x5GaussianBlur(tex, smp, input.uv, 1.0 / w, 1.0 / h);
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
		else if (input.uv.y < 0.8)
		{
			return texHighLum.Sample(smp, (input.uv - float2(0.0, 0.6)) * 5);
		}
		else if (input.uv.y < 1.0)
		{
			return texShrinkHighLum.Sample(smp, (input.uv - float2(0.0, 0.8)) * 5);
		}
	}

	float4 bloomAccum = float4(0.0, 0.0, 0.0, 0.0);
	float2 uvSize = float2(1.0, 0.5);
	float2 uvOfst = float2(0.0, 0.0);

	for (int i = 0; i < 8; ++i)
	{
		bloomAccum += Get5x5GaussianBlur(texShrinkHighLum, smp, 
			input.uv * uvSize + uvOfst, dx, dy);
		uvOfst.y += uvSize.y;
		uvSize *= 0.5;
	}

	return tex.Sample(smp, input.uv) // �ʏ�e�N�X�`��
		+ Get5x5GaussianBlur(texHighLum, smp, input.uv, dx, dy)
		+ saturate(bloomAccum);

	return tex.Sample(smp, input.uv) + Get5x5GaussianBlur(texHighLum, smp, input.uv, dx, dy);

	// �f�B�t�@�[�h�V�F�[�f�B���O�p�̏���
	{
		//float4 normal = texNormal.Sample(smp, input.uv);
		//normal = normal * 2.0 - 1.0;

		//float3 light = normalize(float3(1.0, -1.0, 1.0));
		//const float ambient = 0.25;
		//float diffB = max(saturate(dot(normal.xyz, -light)), ambient);
		//return tex.Sample(smp, input.uv) * float4(diffB, diffB, diffB, 1.0);
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

		//return (ret / 256);
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