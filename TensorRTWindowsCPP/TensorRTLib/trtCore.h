/*****************************************************************************
* @author : FelixFu
* @date : 2021/10/10 14:40
* @last change :
* @description : TensorRT ���Ŀ�
*****************************************************************************/
#pragma once

//#ifdef type_trt_core_api_exports
//#define type_trt_core_api __declspec(dllexport)
//#else
//#define type_trt_core_api __declspec(dllimport)
//#endif

#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "params.h"


// \! API�ӿ�
//class TYPE_TRT_CORE_API TRTCORE {
class TRTCORE{
public:
	typedef struct TRTCore_ctx TRTCore_ctx; // ������Ҫ�õ���һ���ṹ�壬������params��engine��pool��, ������engine.h��

	// @param:params     ��ʼ������
	// @param:nErrnoFlag ��ʼ�������룬�����params.h
	std::shared_ptr<TRTCore_ctx> init(const Params& params, int& nErrnoFlag);

	// \! ����
	int classify(std::shared_ptr<TRTCore_ctx> ctx, const std::vector<cv::Mat> &cvImages, std::vector<std::vector<ClassifyResult>>& outputs);

	// \! �ָ�
	// if verbose=true,return the probability graph, else return the class id image
	int segment(std::shared_ptr<TRTCore_ctx> ctx, const std::vector<cv::Mat> &cvImages, std::vector<cv::Mat>& outputs, bool verbose = false);

	// \! Ŀ��������
	int detect(std::shared_ptr<TRTCore_ctx> ctx, const std::vector<cv::Mat> &cvImages, std::vector<std::vector<BBox>>& outputs);

	// \! ���GPU����
	int getDevices();

	// \! ���BatchSize, Channels, H, W
	int getDims(std::shared_ptr<TRTCore_ctx> ctx, int& nBatch, int& nChannels, int& nHeight, int &nWidth);
};

