/*****************************************************************************
* @author : FelixFu
* @date : 2021/10/10 14:40
* @last change :
* @description : TensorRT ���Ŀ�
*****************************************************************************/
#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "loguru.hpp" // https://github.com/emilk/loguru
#include "params.h"


// \! TRTCORE�ӿ�
class TRTCORE{
public:
	typedef struct TRTCore_ctx TRTCore_ctx; // ������Ҫ�õ���һ���ṹ�壬������params��engine��pool��, ������engine.h��

	// \! ��ʼ��
	// \@param params     ��ʼ������
	// \@param nErrnoFlag ��ʼ�������룬�����params.h
	std::shared_ptr<TRTCore_ctx> init(const Params& params, int& nErrnoFlag);

	// \! ����
	// \@param ctx:ִ��������
	// \@param vInCoreImages:����ͼ���б�Mat��ʽ
	// \@param vvOutClsRes:��������ClassifyResult��ʽ
	int classify(std::shared_ptr<TRTCore_ctx> ctx,const std::vector<cv::Mat> &cvImages,std::vector<std::vector<ClassifyResult>>& outputs);

	// \! �ָ�
	// \@param ctx: ִ��������
	// \@param vInCoreImages: ����ͼƬvector��cvImage
	// \@param vOutCoreImages:���ͼƬvector��cvImage
	// \@param verbose: ���Ϊtrue,return the probability graph, else return the class id image
	int segment(std::shared_ptr<TRTCore_ctx> ctx,const std::vector<cv::Mat> &cvImages, std::vector<cv::Mat>& outputs, bool verbose = false);

	// \! Ŀ����
	int detect(
		std::shared_ptr<TRTCore_ctx> ctx, 
		const std::vector<cv::Mat> &cvImages, 
		std::vector<std::vector<BBox>>& outputs
	);

	// \! �쳣���
	int anomaly(
		std::shared_ptr<TRTCore_ctx> ctx,
		const std::vector<cv::Mat> &cvImages,
		std::vector<cv::Mat>& outputs
	);

	// \! ��ȡ�Կ�����
	// \@param ctx:ִ��������
	// \@param number:gpu����
	int getNumberGPU(std::shared_ptr<TRTCore_ctx> ctx,int& number);

	// \! ��ȡ����ά��
	// \@param ctx:ִ��������
	// \@param nBatch:batchsize
	// \@param nChannels:channels
	// \@param nHeight:height
	// \@param nWidth:width
	// \@param index:��index�����룬����onnx�ж�����룬��ͨ��index��ָ��
	int getInputDims(std::shared_ptr<TRTCore_ctx> ctx,int & nBatch,	int & nChannels,int & nHeight,int & nWidth,int index=0);

	// \! ��ȡ���ά��
	// \@param ctx��ִ��������
	// \@param nBatch:batchsize
	// \@param nHeight:Height
	// \@param nWidth:Width
	// \@param index:��index�����������onnx�ж���������ͨ��index��ָ��
	int getOutputDims(std::shared_ptr<TRTCore_ctx> ctx,	int& nBatch,int& nHeight,int &nWidth,int index=0);
	
	// \! ��ȡ���ά��
	// \@param ctx��ִ��������
	// \@param nBatch:batchsize
	// \@param nNumClass:NumClass ���������Է���
	// \@param index:��index�����������onnx�ж���������ͨ��index��ָ��
	int getOutputDims2(std::shared_ptr<TRTCore_ctx> ctx,int & nBatch,int & nNumClass,int index = 0);

	// \! ��ȡ��������
	// \@param ctx��ִ��������
	// \@param index:��index�����������onnx�ж���������ͨ��index��ָ��
	std::string getInputNames(std::shared_ptr<TRTCore_ctx> ctx,int index = 0);

	// \! ��ȡ�������
	// \@param ctx��ִ��������
	// \@param index:��index�����������onnx�ж���������ͨ��index��ָ��
	std::string getOutputNames(std::shared_ptr<TRTCore_ctx> ctx,int index = 0);
};

