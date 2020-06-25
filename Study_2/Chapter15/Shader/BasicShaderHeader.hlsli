Texture2D<float4> tex : register(t0);  // 0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> sph : register(t1);  // 1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> spa : register(t2);  // 2�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> toon : register(t3); // 3�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float> lightDepthTex : register(t4); // �V���h�E�}�b�v�p���C�g�[�x�e�N�X�`��

SamplerState smp : register(s0);     // 0�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[
SamplerState smpToon : register(s1); // 1�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[�i�g�D�[���p�j
SamplerComparisonState shadowSmp : register(s2); // �V���h�E�}�b�v�p�̃T���v���[

// ���_�V�F�[�_�[����s�N�Z���V�F�[�_�[�ւ̂����Ɏg�p����\����
struct Output
{
	float4 svpos   : SV_POSITION; // �V�X�e���p���_���W
	float4 normal  : NORMAL0;     // �@���x�N�g��
	float4 vnormal : NORMAL1;     // �r���[�ϊ���̖@���x�N�g��
	float2 uv      : TEXCOORD;    // uv�l�i�e�N�X�`����̍��W 0.0f�`1.0f�j
	uint instNo    : SV_InstanceID;
	float3 ray     : VECTOR;      // �x�N�g��
	float4 tpos    : TPOS;
};

struct PixelOutput
{
	float4 col     : SV_TARGET0;    // �J���[�l���o��
	float4 normal  : SV_TARGET1; // �@�����o��
	float4 highLum : SV_TARGET2; // ���P�x
};

// �萔�o�b�t�@
cbuffer cbuff0 : register(b0)
{
	matrix view;        // �r���[�ϊ��s��
	matrix proj;        // �v���W�F�N�V�����ϊ��s��
	matrix invproj;     // �t�v���W�F�N�V����
	matrix lightCamera; // ���C�g�r���[�v���W�F�N�V����
	matrix shadow;      // �e
	float3 eye;         // ���_
};

cbuffer Transform : register(b1)
{
	matrix world;     // ���[���h�ϊ��s��
	matrix bones[256]; // �{�[���s��
};

cbuffer Material : register(b2)
{
	float4 diffuse;  // �f�B�t���[�Y�F
	float4 specular; // �X�y�L����
	float3 ambient;	 // �A���r�G���g
};