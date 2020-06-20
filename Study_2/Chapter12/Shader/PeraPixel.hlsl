#include "PeraHeader.hlsli"

float4 PeraPS(Output input) : SV_TARGET
{
	float4 col = tex.Sample(smp, input.uv);
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0 / w;
	float dy = 1.0 / h;
	float4 ret = float4(0, 0, 0, 0);

	// �֊s�����o
	{
		ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) *  0; // ����
		ret += tex.Sample(smp, input.uv + float2(      0, -2 * dy)) * -1; // ��
		ret += tex.Sample(smp, input.uv + float2( 2 * dx, -2 * dy)) *  0; // �E��

		ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1; // ��
		ret += tex.Sample(smp, input.uv)                      *  4; // ����
		ret += tex.Sample(smp, input.uv + float2 (2 * dx, 0)) * -1; // �E

		ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) *  0; // ����
		ret += tex.Sample(smp, input.uv + float2(      0, 2 * dy)) * -1; // ��
		ret += tex.Sample(smp, input.uv + float2( 2 * dx, 2 * dy)) *  0; // �E��

		// �F���]
		float y = dot(ret.rgb, float3(0.299, 0.587, 0.114));

		y = pow(1.0 - y, 10);

		y = step(0.2, y); // ���������łĂ���Ƃ�����Ȃ����Ă�̂��ȁH

		return float4(y, y, y, 1.0);
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
		//return col;
	}
}