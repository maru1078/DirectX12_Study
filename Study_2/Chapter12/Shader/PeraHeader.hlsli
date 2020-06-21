Texture2D<float4> tex : register(t0); // �ʏ�e�N�X�`��
Texture2D<float4> effectTex : register(t1); // �c�ݗp�@���}�b�v
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