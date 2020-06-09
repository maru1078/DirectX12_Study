#include "peraHeader.hlsli"

// �s�N�Z���V�F�[�_�[
float4 ps(Output input) : SV_TARGET
{
	//return float4(input.uv, 1, 1);

	// �ʏ�`��
	{
		//return tex.Sample(smp, input.uv);
    }

	float4 col = tex.Sample(smp, input.uv);

	// ���m�N����
    {
	    //float Y = dot(col.rgb, float3(0.299, 0.587, 0.114));

	    //return float4(Y, Y, Y, 1);
    }

	// �F�̔��]
	{
		//return float4(float3(1.0f, 1.0f, 1.0f) - col.rgb, col.a);
	}

	// ���g�����ۂ�
	{
		//return float4(col.rgb - fmod(col.rgb, 0.25), col.a);
	}

	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);

	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	// ��f�̕��ω��ɂ��ڂ���
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)); // ����
		//ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)); // ��
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)); // �E��

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)); // ��
		//ret += tex.Sample(smp, input.uv); // ����
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)); // �E

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)); // ����
		//ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)); // ��
		//ret += tex.Sample(smp, input.uv + float2(2 * dy, 2 * dy)); // �E��
		//return ret / 9.0f;
	}

	//�G���{�X���H
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 2; // ����
		//ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)); // ��
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0; // �E��

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)); // ��
		//ret += tex.Sample(smp, input.uv); // ����
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1; // �E

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 0; // ����
		//ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1; // ��
		//ret += tex.Sample(smp, input.uv + float2(2 * dy, 2 * dy)) * -2; // �E��

		//return ret;
	}

	// �V���[�v�i�G�b�W�̋����j
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 0; // ����
		//ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)) * -1; // ��
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0; // �E��

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1; // ��
		//ret += tex.Sample(smp, input.uv) * 5; // ����
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1; // �E

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 0; // ����
		//ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1; // ��
		//ret += tex.Sample(smp, input.uv + float2(2 * dy, 2 * dy)) * 0; // �E��

		//return ret;
	}

	// �֊s���̒��o�i�F�t���j
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 0; // ����
		//ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)) * -1; // ��
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0; // �E��

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1; // ��
		//ret += tex.Sample(smp, input.uv) * 4; // ����
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1; // �E

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 0; // ����
		//ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1; // ��
		//ret += tex.Sample(smp, input.uv + float2(2 * dy, 2 * dy)) * 0; // �E��

		//return ret;
	}

	// �֊s���̒��o�i���̐���j
	{
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 0; // ����
		//ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)) * -1; // ��
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0; // �E��

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1; // ��
		//ret += tex.Sample(smp, input.uv) * 4; // ����
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1; // �E

		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 0; // ����
		//ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1; // ��
		//ret += tex.Sample(smp, input.uv + float2(2 * dy, 2 * dy)) * 0; // �E��

		//// ���m�N���ɂ���
		//float Y = dot(ret.rgb, float3(0.299, 0.587, 0.114));
		//// ���]�i�������ɂ���j
		//Y = 1.0 - Y;
		//// ���̐F��Z������
		//Y = pow(Y, 10);
		//Y = step(0.2, Y);

		//return float4(Y, Y, Y, col.a);
	}

	// �K�E�V�A���ڂ����i�ȒP�Ȃق��j
	{
		//// �ŏ�i
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 1 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 2 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, 2 * dy)) * 6 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, 2 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 2 * dy)) * 1 / 256;

		//// 1��i
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 1 * dy)) *  4 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, 1 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, 1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 1 * dy)) *  4 / 256;

		//// ���i
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0 * dy)) *  6 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 0 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, 0 * dy)) * 36 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, 0 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, 0 * dy)) *  6 / 256;

		//// 1���i
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -1 * dy)) *  4 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, -1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, -1 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, -1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, -1 * dy)) *  4 / 256;

		//// �ŉ��i
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 1 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, -2 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 0 * dx, -2 * dy)) * 6 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 1 * dx, -2 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2( 2 * dx, -2 * dy)) * 1 / 256;

		//return ret;
	}

	// �K�E�V�A���ڂ����i������j
	{
		//// �ŏ�i
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 1 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 2 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2(0 * dx, 2 * dy)) * 6 / 256;
		//ret += tex.Sample(smp, input.uv + float2(1 * dx, 2 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * 1 / 256;

		//// 1��i
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 1 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2(0 * dx, 1 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2(1 * dx, 1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 1 * dy)) * 4 / 256;

		//// ���i
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0 * dy)) * 6 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, 0 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2(0 * dx, 0 * dy)) * 36 / 256;
		//ret += tex.Sample(smp, input.uv + float2(1 * dx, 0 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, 0 * dy)) * 6 / 256;

		//// 1���i
		//ret += tex.Sample(smp, input.uv + float2(-2 * dx, -1 * dy)) * 4 / 256;
		//ret += tex.Sample(smp, input.uv + float2(-1 * dx, -1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2(0 * dx, -1 * dy)) * 24 / 256;
		//ret += tex.Sample(smp, input.uv + float2(1 * dx, -1 * dy)) * 16 / 256;
		//ret += tex.Sample(smp, input.uv + float2(2 * dx, -1 * dy)) * 4 / 256;

		//// �ŉ��i
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

	// �V���h�E�}�b�v
	{
		//float dep = pow(depthTex.Sample(smp, input.uv), 20);
		//return float4(dep, dep, dep, 1);
	}

	// �}���`�����_�[�^�[�Q�b�g
	{
		if (input.uv.x < 0.2 && input.uv.y < 0.2) // �[�x�o��
		{
			float depth = depthTex.Sample(smp, input.uv * 5);
			depth = pow(depth, 50);
			return float4(depth, depth, depth, 1);
		}
		else if (input.uv.x < 0.2 && input.uv.y < 0.4) // ���C�g����̐[�x�o��
		{
			//return float4(1.0f, 0.0f, 0.0f, 1.0f);
			float depth = lightDepthTex.Sample(smp, (input.uv - float2(0, 0.2)) * 5);

			//depth = 1 - depth;
			return float4(depth, depth, depth, 1);
		}
		else if (input.uv.x < 0.2 && input.uv.y < 0.6) // �@���o��
		{
			//return float4(1.0f, 1.0f, 0.0f, 1.0f);
			return texNormal.Sample(smp, (input.uv - float2(0, 0.4)) * 5);
		}
	}

	return tex.Sample(smp, input.uv);

	// �f�B�t�@�[�h�V�F�[�f�B���O
	{
		//float4 normal = texNormal.Sample(smp, input.uv);
        //normal = normal * 2.0f - 1.0f;
        
        //float3 light = normalize(float3(1.0f, -1.0f, 1.0f));
        //const float ambient = 0.25f;
        //float diffB = max(saturate(dot(normal.xyz, -light)), ambient);
        
        //return tex.Sample(smp, input.uv) * float4(diffB, diffB, diffB, 1);
	}
}