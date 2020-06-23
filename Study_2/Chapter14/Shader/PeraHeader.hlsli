Texture2D<float4> tex              : register(t0); // �ʏ�e�N�X�`��
Texture2D<float4> texNormal        : register(t1); // �@���e�N�X�`��
Texture2D<float4> texHighLum       : register(t2); // ���P�x
Texture2D<float4> texShrinkHighLum : register(t3); // �k���o�b�t�@�[���P�x
Texture2D<float4> effectTex        : register(t4); // �c�ݗp�@���}�b�v
Texture2D<float> depthTex          : register(t5); // �[�x�e�N�X�`��
Texture2D<float> lightDepthTex     : register(t6); // ���C�g�f�v�X

SamplerState smp : register(s0); // �T���v���[

struct Output
{
	float4 svpos : SV_POSITION;
	float2 uv    : TEXCOORD;
};

cbuffer PostEffect : register(b0)
{
	float4 bkweights[2];
};