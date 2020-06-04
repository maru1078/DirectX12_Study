// ���_�V�F�[�_�[����s�N�Z���V�F�[�_�[�ւ̂����Ɏg�p����\����
struct Output
{
	float4 svpos : SV_POSITION; // �V�X�e���p���_���W
	float4 pos : POSITION; // ���_���W
	float4 normal : NORMAL0; // �@���x�N�g��
	float4 vnormal : NORMAL1; // �@���x�N�g��
	float2 uv    : TEXCOORD; // uv�l
	float3 ray : VECTOR; // �x�N�g��
	uint instNo : SV_InstanceID;
};

Texture2D<float4> tex : register(t0); // 0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> sph : register(t1); // 1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> spa : register(t2); // 2�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> toon : register(t3); // 3�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`���i�g�D�[���j

// �[�x���ؗp
//Texture2D<float> depthTex : register(t2);

SamplerState smp : register(s0);      // 0�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[
SamplerState smpToon : register(s1);      // 1�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[

// �萔�o�b�t�@
cbuffer cbuff0 : register(b0)
{
	matrix view;   // �r���[�s��
	matrix proj;   // �v���W�F�N�V�����s��
	matrix lightCamera; // ���C�g�r���[�v���W�F�N�V����
	matrix shadow; // �e�s��
	float3 eye;    // ���_
};

cbuffer Transform : register(b1)
{
	matrix world;      // ���[���h�ϊ��s��
	matrix bones[256]; // �{�[���s��
}

// �萔�o�b�t�@1
// �}�e���A���p
cbuffer Material : register(b2)
{
	float4 diffuse;  // �f�B�t���[�Y�F
	float4 specular; // �X�y�L����
	float3 ambient;  // �A���r�G���g
};