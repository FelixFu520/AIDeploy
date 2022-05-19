/*****************************************************************************
* @author : FelixFu
* @date : 2021/10/10 14:40
* @last change :
* @description : TRTCORE ���API����TRTCORE�Ͻ�һ����װ��ȥ����opencv������
*****************************************************************************/
#ifndef TRTAPI_H
#define TRTAPI_H

#if defined (TYPE_TRTAPI_API_EXPORTS)
#define TYPE_TRTAPI_API __declspec(dllexport)
#else
#define TYPE_TRTAPI_API __declspec(dllimport)
#endif

//#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "params.h"


// \! ---------����ӿ� ���롢�����ʽ Start------------
// \! ͼ��Ļ�������
class CoreImage
{
public:
	void SetValue(int channal, int width, int height, int step, unsigned char* data)
	{
		channal_ = channal;
		width_ = width;
		height_ = height;
		imagestep_ = step;
		imagedata_ = data;
	};

public:
	//ͼ��ͨ��
	int channal_;
	//��	
	int width_;
	//��
	int height_;
	//ÿ���ֽ���
	int imagestep_;
	//ͼ������
	unsigned char *imagedata_;
};

// \! API�ӿ�
class TRTCORE;  // ������ӿ�
class TYPE_TRTAPI_API TRTAPI { // Ӧ�ò�ӿ�
public:
	typedef struct TRTCore_ctx TRTCore_ctx;   //������Ҫ�õ���һ���ṹ�壬������params��engine��pool��
	TRTAPI();

	~TRTAPI();
	// @param:params     ��ʼ������
	// @param:nErrnoFlag ��ʼ�������룬�����params.h
	std::shared_ptr<TRTCore_ctx> init(const Params& params, int& nErrnoFlag); 	// \! ��ʼ��

	// \! ����
	int classify(std::shared_ptr<TRTCore_ctx> ctx, const std::vector<CoreImage*> &vInCoreImages, std::vector<std::vector<ClassifyResult>>& vvOutClsRes);

	// \! �ָ�
	int segment(std::shared_ptr<TRTCore_ctx> ctx, const std::vector<CoreImage*> &vInCoreImages, std::vector<CoreImage*>& vOutCoreImages);

	// \! Ŀ��������
	int detect(std::shared_ptr<TRTCore_ctx> ctx, const std::vector<CoreImage*> &vInCoreImages, std::vector<std::vector<BBox>>& vvOutBBoxs);

	// ��ȡ�Կ�����
	int getDevices();

	int getDims(std::shared_ptr<TRTCore_ctx> ctx, int& nBatch, int& nChannels, int& nHeight, int &nWidth);

private:
	TRTCORE *m_pTRTCore; // Ϊ�˷��������������������������ӿڣ�������ӿڡ�Ӧ�ò�ӿڡ�Ӧ�ò�ӿ����ڿ�����֮�Ϸ�װ�Ķ���ӿڡ�
};
#endif