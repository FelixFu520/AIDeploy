/*****************************************************************************
* @author : FelixFu
* @date : 2021/10/10 14:40
* @last change :
* @description : ORTCore ���Ŀ�
*****************************************************************************/
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "params.h"
#include "engine.h"


// \! ���Ľӿ�
class ORTCORE {
public:
	typedef struct ORTCore_ctx ORTCore_ctx; // ִ��������

	// \! ��ʼ��
	std::shared_ptr<ORTCore_ctx> init(
		const Params& params,	// @param:params     ��ʼ������
		int& nErrnoFlag			// @param:nErrnoFlag ��ʼ�������룬�����params.h
	);

	// \! ����
	int classify(
		std::shared_ptr<ORTCore_ctx> ctx,
		const std::vector<cv::Mat> &cvImages, 
		std::vector<std::vector<ClassifyResult>>& outputs
	);

	// \! �쳣���
	int anomaly(
		std::shared_ptr<ORTCore_ctx> ctx, // ִ��������
		const std::vector<cv::Mat> &cvImages,  // ����ͼƬ
		std::vector<cv::Mat>& outputs		// ���ͼƬ
	);

	// \! �ָ�
	int segment(
		std::shared_ptr<ORTCore_ctx> ctx,
		const std::vector<cv::Mat> &cvImages, 
		std::vector<cv::Mat>& outputs
	);
	// \! Ŀ��������
	int detect(
		std::shared_ptr<ORTCore_ctx> ctx,
		const std::vector<cv::Mat> &cvImages, 
		std::vector<std::vector<BBox>>& outputs
	);

	// \! ���BatchSize, Channels, H, W
	int getInputDimsK(
		std::shared_ptr<ORTCore_ctx> ctx, 
		int& nBatch,
		int& nChannels,
		int& nHeight, 
		int &nWidth
	);

	// \! ���BatchSize, H, W
	int getOutputDimsK(
		std::shared_ptr<ORTCore_ctx> ctx,
		int& nBatch,
		int& nHeight,
		int &nWidth
	);

	// \! ���BatchSize, NumClass
	int getOutputDimsK(
		std::shared_ptr<ORTCore_ctx> ctx,
		int& nBatch,
		int &numClass
	);
	
};

