#include <opencv2/opencv.hpp> // opencv include
#include <iostream> // system include
#include <Windows.h>


#include "F_log.h"
#include "ORTAPI.h"
#include "PARAMS.h"

// \! ���ڼ���ʱ����
class TimeTick
{
public:
	TimeTick(void)
	{
		QueryPerformanceFrequency(&mFrequency);
	};  //���캯��

private:
	LARGE_INTEGER mStartTime;
	LARGE_INTEGER mEndTime;
	LARGE_INTEGER mCurrentTime;

	LARGE_INTEGER mFrequency;  // CPUƵ�� ��ʱ�ľ��ȸ�Ƶ���йأ��ҵ���Ƶ����10e8����ʱ����Ϊ10���뼶��

public:
	double mInterval;  // ���

public:
	void start()
	{
		QueryPerformanceCounter(&mStartTime);
	};
	void end()
	{
		QueryPerformanceCounter(&mEndTime);

		mInterval = ((double)mEndTime.QuadPart - (double)mStartTime.QuadPart) / (double)mFrequency.QuadPart;  //�룬10e-8����

	};
	double tick()
	{
		QueryPerformanceCounter(&mCurrentTime);
		return (double)mCurrentTime.QuadPart / (double)mFrequency.QuadPart;
	};
};


int main()         // ����ֵΪ���ʹ��ε�main����. ��������ʹ�û�ʹ��argc��argv����
{
	ORTAPI ortAPI;
	// 0 �������;	���߳�
	// 1 �ָ����;	���߳�
	// 2 �쳣������; ���߳�
	// 3 �쳣������; ���߳�
	
	int flag_s = 3;
	switch (flag_s)
	{
	case 0:
	{
		// 1.ģ�Ͳ���
		Params params;
		params.onnxFilePath = "E:\\AIDeploy\\Env\\DemoData\\classification\\onnxs\\PZb0b8.onnx";
		params.log_path = "D:/log.txt";
		params.netType = FF_CLS;
		params.meanValue = { 0, 0, 0 };
		params.stdValue = { 1, 1, 1 };
		params.sessionThread = 1;

		// 2. ��ʼ��
		int errorFlag;
		auto ctx = ortAPI.init(params, errorFlag);

		// 3. ��������
		std::vector<std::string> file_names;
		cv::glob("E:/AIDeploy/Env/DemoData/classification/images/*.bmp", file_names);
		int batch_size, channels, height, width;
		ortAPI.getInputDimsK(ctx, batch_size, channels, height, width); // ���onnx�е�����ά��

		for (int nn = 0; nn < file_names.size() / batch_size; nn++) { // ����ͼƬ�ܶ࣬��Ҫѭ�����������
			std::vector<CoreImage*> inputs; 
			std::vector<std::vector<ClassifyResult>> outputs;

			std::vector<cv::Mat> inputsCvImage;// ���CV::Mat �ڴ˴�ѭ���б����ڴ�
			CoreImage *inputCoreImage = new CoreImage[batch_size];// ���CoreImage�����飬�ڴ˴�ѭ���б������ڴ���

			for (int b = 0; b < batch_size; b++) {  // ÿ�δ�����batchsize��batchsize��С���Ϊonnx�е�batchsize��С
				inputsCvImage.push_back(cv::imread(file_names[nn * batch_size + b], cv::IMREAD_GRAYSCALE));
				inputCoreImage[b].SetValue(
					inputsCvImage[b].channels(),
					inputsCvImage[b].cols,
					inputsCvImage[b].rows,
					inputsCvImage[b].step,
					(unsigned char *)inputsCvImage[b].data
				);
				inputs.push_back(&inputCoreImage[b]);
			}
			
			// 4. ����
			TimeTick time;
			time.start();
			ortAPI.classify(ctx, inputs, outputs);
			time.end();
			std::cout << "Infer Time : " << time.mInterval * 1000 << "ms" << std::endl;

			for (int b = 0; b < batch_size; b++) {
				std::cout << file_names[nn*batch_size + b] << " ..................... " << "outputs:::: top1=";
				std::cout << std::to_string(outputs[b][0].first) << ":" << std::to_string(outputs[b][0].second);
				std::cout << "    top2=";
				std::cout << std::to_string(outputs[b][1].first) << ":" << std::to_string(outputs[b][1].second) << std::endl;
			}
		}// ����ͼƬ�ܶ࣬��Ҫѭ�����������
		break;
	}
	case 1:
	{
		// 1.ģ�Ͳ���
		Params params;
		params.onnxFilePath = "E:/AIDeploy/Env/DemoData/segmentation/onnxs/PSPNet2_resnet50.onnx";
		params.log_path = "D:/log.txt";
		params.netType = FF_SEG;
		params.meanValue = { 0.45734706, 0.43338275, 0.40058118 };
		params.stdValue = { 0.23965294, 0.23532275, 0.2398498 };
		params.sessionThread = 1;

		// 2. ��ʼ��
		int errorFlag;
		auto ctx = ortAPI.init(params, errorFlag);

		// 3. ��������
		std::vector<std::string> file_names;
		cv::glob("E:/AIDeploy/Env/DemoData/segmentation/images/*.jpg", file_names);
		int batch_size, channels, height, width;
		ortAPI.getInputDimsK(ctx, batch_size, channels, height, width); // ���onnx�е�����ά��
		int batch_size_output, height_output, width_output;
		ortAPI.getOutputDimsK(ctx, batch_size_output, height_output, width_output);// ���onnx�е����ά��

		for (int nn = 0; nn < file_names.size() / batch_size; nn++) { // ����ͼƬ�ܶ࣬��Ҫѭ�����������
			std::vector<CoreImage*> inputs; // API�ӿڵ�����
			std::vector<CoreImage*> outputs; // API�ӿڵ�����

			// �������ʵ���ͼƬ���ڴ�ռ�
			std::vector<cv::Mat> inputsCvImage; // ���CV::Mat �ڴ˴�ѭ���б����ڴ�
			std::vector<cv::Mat> outputsCvImage; // ���CV::Mat �ڴ˴�ѭ���б����ڴ�

			// ����ǽ�CVͼƬת��CoreImageͼƬ���ڴ�ռ�
			CoreImage *inputsCoreImage = new CoreImage[batch_size];// ���CoreImage�����飬�ڴ˴�ѭ���б������ڴ���
			CoreImage *outputsCoreImage = new CoreImage[batch_size];// ���CoreImage�����飬�ڴ˴�ѭ���б������ڴ���

			// ��һ��batchsizeͼƬ����-->CVImage-->CoreImage�����մ�ŵ�inputsCvImage��outputsCvImage��
			for (int b = 0; b < batch_size; b++) {  // ÿ�δ�����batchsize��batchsize��С���Ϊonnx�е�batchsize��С
				// 3.1 ����batch_size�е�һ��ͼƬ--����
				inputsCvImage.push_back(cv::imread(file_names[nn * batch_size + b], cv::IMREAD_COLOR));
				inputsCoreImage[b].SetValue(
					inputsCvImage[b].channels(),
					inputsCvImage[b].cols,
					inputsCvImage[b].rows,
					inputsCvImage[b].step,
					(unsigned char *)inputsCvImage[b].data
				);
				inputs.push_back(&inputsCoreImage[b]);

				// 3.2 ����batch_size�е�һ��ͼƬ--���
				outputsCvImage.push_back(cv::Mat::zeros(cv::Size(height_output, width_output), CV_8UC1));
				outputsCoreImage[b].SetValue(
					outputsCvImage[b].channels(),
					outputsCvImage[b].cols,
					outputsCvImage[b].rows,
					outputsCvImage[b].step,
					(unsigned char *)outputsCvImage[b].data
				);
				outputs.push_back(&outputsCoreImage[b]);
			}

			// 4. ����
			TimeTick time;
			time.start();
			ortAPI.segment(ctx, inputs, outputs);
			time.end();
			std::cout << "Infer Time : " << time.mInterval * 1000 << "ms" << std::endl;

			// 5. �Խ�����д���
			// ͬ��õ�mask�����·��
			std::vector<std::string> outputs_files;
			for (auto files : file_names) {
				files.replace(files.find(".jpg"), 4, ".png");
				outputs_files.push_back(files);
			}
			for (int k = 0; k < outputs.size(); k++)
			{
				cv::Mat tmp = cv::Mat(
					outputs[k]->height_,
					outputs[k]->width_,
					CV_8UC1,
					outputs[k]->imagedata_,
					outputs[k]->imagestep_).clone();
				cv::resize(tmp, tmp, cv::Size(inputsCvImage[k].cols, inputsCvImage[k].rows), cv::INTER_NEAREST);
				cv::imwrite(outputs_files[k], tmp);
			}
		}// ����ͼƬ�ܶ࣬��Ҫѭ�����������
		break;
	}
	case 2:
	{
		// ģ�Ͳ���
		Params params;
		params.onnxFilePath = "E:\\AIDeploy\\Env\\DemoData\\anomaly\\onnxs\\PaDiM2_b36.onnx";
		params.log_path = "D:/log.txt";
		params.netType = FF_ANOMALY;
		params.meanValue = { 0.335782, 0.335782, 0.335782 };
		params.stdValue = { 0.256730, 0.256730, 0.256730 };
		params.sessionThread = 1;

		// ��ʼ��
		int flag;
		auto ctx = ortAPI.init(params, flag);

		// ��������
		std::vector<std::string> file_names;
		cv::glob("E:/AIDeploy/Env/DemoData/anomaly/images/*.bmp", file_names);
		int batch_size, channels, height, width;
		ortAPI.getInputDimsK(ctx, batch_size, channels, height, width);

		for (int nn = 0; nn < file_names.size() / batch_size; nn++) { // ����ͼƬ�ܶ࣬��Ҫѭ�����������
			std::vector<CoreImage*> inputs1, inputs2; // �߳�1, ..., n ������
			std::vector<CoreImage*> outputs1, outputs2;//�߳�1, ..., n�����

			std::vector<cv::Mat> inputs1_tmp, inputs2_tmp; // ���CV::Mat �ڴ˴�ѭ���б����ڴ�
			CoreImage *inputs_core_images1 = new CoreImage[batch_size];// ���CoreImage�����飬�ڴ˴�ѭ���б������ڴ���
			//CoreImage *inputs_core_images2 = new CoreImage[batch_size];// ���CoreImage�����飬�ڴ˴�ѭ���б������ڴ���
			std::vector<cv::Mat> outputs1_tmp, outputs2_tmp; // ���CV::Mat �ڴ˴�ѭ���б����ڴ�
			CoreImage *outputs_core_images1 = new CoreImage[batch_size];// ���CoreImage�����飬�ڴ˴�ѭ���б������ڴ���


			for (int b = 0; b < batch_size; b++) {  // ÿ�δ�����batchsize��batchsize��С���Ϊonnx�е�batchsize��С
				inputs1_tmp.push_back(cv::imread(file_names[nn * batch_size + b], cv::IMREAD_COLOR));
				//inputs2_tmp.push_back(cv::imread(file_names[nn * batch_size + b], cv::IMREAD_GRAYSCALE));

				inputs_core_images1[b].SetValue(inputs1_tmp[b].channels(), inputs1_tmp[b].cols, inputs1_tmp[b].rows, inputs1_tmp[b].step, (unsigned char *)inputs1_tmp[b].data);
				//inputs_core_images2[b].SetValue(inputs2_tmp[b].channels(), inputs2_tmp[b].cols, inputs2_tmp[b].rows,inputs2_tmp[b].step, (unsigned char *)inputs2_tmp[b].data);

				inputs1.push_back(&inputs_core_images1[b]);
				//inputs2.push_back(&inputs_core_images2[b]);
				
				outputs1_tmp.push_back(cv::Mat::zeros(cv::Size(224, 224), CV_8UC1));
				outputs_core_images1[b].SetValue(outputs1_tmp[b].channels(), outputs1_tmp[b].cols, outputs1_tmp[b].rows, outputs1_tmp[b].step, (unsigned char *)outputs1_tmp[b].data);
				outputs1.push_back(&outputs_core_images1[b]);
			}

			TimeTick time;
			time.start();
			std::thread obj1(&ORTAPI::anomaly, std::ref(ortAPI), ctx, std::ref(inputs1), std::ref(outputs1), 0.39915153,255);
			//std::thread obj2(&LUSTERTRT::anomaly, std::ref(lustertrt), ctx, std::ref(inputs2), std::ref(outputs2));

			obj1.join();
			//obj2.join();

			time.end();
			std::cout << "infer Time : " << time.mInterval * 1000 << "ms" << std::endl;

			// ͬ��õ�mask�����·��
			std::vector<std::string> outputs_files;
			for (auto files : file_names) {
					std::string tmp = files;
					tmp.replace(files.find(".bmp"), 4, ".png");
				outputs_files.push_back(tmp);
			}
			for (int k = 0; k < outputs1.size(); k++)
			{
				cv::Mat tmp = cv::Mat(
					outputs1[k]->height_,
					outputs1[k]->width_,
					//CV_32FC1,
					CV_8UC1,
					outputs1[k]->imagedata_,
					outputs1[k]->imagestep_
				).clone();

				cv::imwrite(outputs_files[k], tmp);
			}
		}// ����ͼƬ�ܶ࣬��Ҫѭ�����������
		break;
	}
	case 3:
	{
		// 1. ģ�Ͳ�������
		Params params;
		params.onnxFilePath = "E:\\AIDeploy\\Env\\DemoData\\anomaly\\onnxs\\PaDiM2_b8.onnx";
		params.log_path = "D:/log.txt";
		params.netType = FF_ANOMALY;
		params.meanValue = { 0.335782, 0.335782, 0.335782 };
		params.stdValue = { 0.256730, 0.256730, 0.256730 };
		params.sessionThread = 1;

		// 2. ��ʼ��ortAPI
		int errorFlag;
		auto ctx = ortAPI.init(params, errorFlag);

		// 3. ��������
		std::vector<std::string> file_names;
		cv::glob("E:/AIDeploy/Env/DemoData/anomaly/images_16/*.bmp", file_names);
		int batch_size, channels, height, width;
		ortAPI.getInputDimsK(ctx, batch_size, channels, height, width); // ���onnx�е�����ά��

		for (int nn = 0; nn < file_names.size() / batch_size; nn++) { // ����ͼƬ�ܶ࣬��Ҫѭ�����������
			std::vector<CoreImage*> inputs; // API�ӿڵ�����
			std::vector<CoreImage*> outputs; // API�ӿڵ�����
			// �������ʵ���ͼƬ���ڴ�ռ�
			std::vector<cv::Mat> inputsCvImage; // ���CV::Mat �ڴ˴�ѭ���б����ڴ�
			std::vector<cv::Mat> outputsCvImage; // ���CV::Mat �ڴ˴�ѭ���б����ڴ�
			// ����ǽ�CVͼƬת��CoreImageͼƬ���ڴ�ռ�
			CoreImage *inputCoreImage = new CoreImage[batch_size];// ���CoreImage�����飬�ڴ˴�ѭ���б������ڴ���
			CoreImage *outputsCoreImage = new CoreImage[batch_size];// ���CoreImage�����飬�ڴ˴�ѭ���б������ڴ���

			// ��һ��batchsizeͼƬ����-->CVImage-->CoreImage�����մ�ŵ�inputsCvImage��outputsCvImage��
			for (int b = 0; b < batch_size; b++) {  // ÿ�δ�����batchsize��batchsize��С = onnx�е�batchsize��С
				// 3.1 ����batch_size�е�һ��ͼƬ--����
				inputsCvImage.push_back(cv::imread(file_names[nn * batch_size + b], cv::IMREAD_COLOR));// �õ�CV��ʽͼƬ�������ڴ�
				inputCoreImage[b].SetValue(
					inputsCvImage[b].channels(), 
					inputsCvImage[b].cols, 
					inputsCvImage[b].rows, 
					inputsCvImage[b].step, 
					(unsigned char *)inputsCvImage[b].data
				);// �õ�CoreImage��ʽͼƬ�������ڴ�
				inputs.push_back(&inputCoreImage[b]);//ת�������ʽ

				// 3.2 ����batch_size�е�һ��ͼƬ--���
				outputsCvImage.push_back(cv::Mat::zeros(cv::Size(224, 224), CV_8UC1));
				outputsCoreImage[b].SetValue(
					outputsCvImage[b].channels(),
					outputsCvImage[b].cols,
					outputsCvImage[b].rows,
					outputsCvImage[b].step,
					(unsigned char *)outputsCvImage[b].data
				);
				outputs.push_back(&outputsCoreImage[b]);
			}

			// 4. ����
			TimeTick time;
			time.start();
			ortAPI.anomaly(ctx, inputs, outputs, 0.39915153, 255);
			time.end();
			std::cout << "Infer Time : " << time.mInterval * 1000 << "ms" << std::endl;

			// 5. �Խ�����д���
			// ͬ��õ�mask�����·��
			std::vector<std::string> outputs_files;
			for (auto files : file_names) {
				//std::string tmp = files;
				files.replace(files.find(".bmp"), 4, ".png");
				outputs_files.push_back(files);
			}
			for (int k = 0; k < outputs.size(); k++)
			{
				cv::Mat tmp = cv::Mat(
					outputs[k]->height_,
					outputs[k]->width_,
					CV_8UC1,
					outputs[k]->imagedata_,
					outputs[k]->imagestep_).clone();

				cv::imwrite(outputs_files[batch_size*nn+k], tmp);
			}
		}// ����ͼƬ�ܶ࣬��Ҫѭ�����������
		break;
	}
	}
}