#include "utils.h"

// \! ���ࡢ�쳣���ǰ����
int normalization(
	std::vector<cv::Mat>& cv_images, //����ͼ��
	std::vector<float> &input_tensor_values, //Ŀ���ַ
	std::shared_ptr<ORTCore_ctx> ctx, //ִ��������
	Ort::MemoryInfo &memory_info// ort����
)
{
	// 1.��־�����ʼ��
	YLog ortLog(YLog::INFO, ctx.get()->params.log_path, YLog::ADD);

	// 2.��ctx�л��onnx������Ϣ �� params
	std::vector<int64_t> inputDims = ctx.get()->session->mInputDims[0];		// ��һ������ά�ȣ�session��
	int session_batch{ 0 }, session_channels{ 0 }, session_height{ 0 }, session_width{ 0 };
	session_batch = inputDims[0];	// ���batchsize��session��
	session_channels = inputDims[1];// ���channels��session��
	session_height = inputDims[2];// ���height��session��
	session_width = inputDims[3];// ���width��session��
	size_t input_tensor_size = session_batch * session_channels * session_height * session_width;  // ���ͼƬ�ߴ�
	Params params = ctx.get()->params;// ��ctx�л��params

	// 3.read image
	std::vector<float> dst_data;
	if (session_channels == 1) {// ͨ����Ϊ1
		for (int i = 0; i < cv_images.size(); i++) {
			cv_images[i].convertTo(cv_images[i], CV_32FC1, 1 / 255.0);
			cv_images[i] = (cv_images[i] - params.meanValue[0]) / params.stdValue[0];
			std::vector<float> data = std::vector<float>(cv_images[i].reshape(1, 1));
			dst_data.insert(dst_data.end(), data.begin(), data.end());
		}
	}
	else if (session_channels == 3) {
		// �µķ������ο���https://blog.csdn.net/juluwangriyue/article/details/123041695
		for (int n = 0, index = 0; n < cv_images.size(); n++)
		{
			cv_images[n].convertTo(cv_images[n], CV_32F, 1.0 / 255);
			std::vector<cv::Mat> bgrChannels(3);//����������HWC->CHW
			cv::split(cv_images[n], bgrChannels);
			for (int i = 0; i < cv_images[n].channels(); i++)
			{
				bgrChannels[i] -= params.meanValue[i];  // mean
				bgrChannels[i] /= params.stdValue[i];   // std
			}
			for (int i = 0; i < cv_images[n].channels(); i++)  // BGR2RGB, HWC->CHW
			{
				std::vector<float> data = std::vector<float>(bgrChannels[2 - i].reshape(1, cv_images[n].cols * cv_images[n].rows));
				dst_data.insert(dst_data.end(), data.begin(), data.end());
			}
		}
	}
	else {
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "Normalization", "��֧�ֵ�ͼ������");
		return FF_ERROR_INPUT;
	}
	// ��dst_data��ֵ��input_tensor_values
	for (unsigned int i = 0; i < input_tensor_size; i++)
		input_tensor_values[i] = dst_data[i];
}


// �Ƚ�Pair
bool PairCompare(const std::pair<float, int>& lhs, const std::pair<float, int>& rhs)
{
	return lhs.first > rhs.first;
}
// ���������з�������N��÷����,�����С�������Ŀc���������СN
std::vector<int> Argmax(const std::vector<float>& v, int N)
{
	// ��v��һ��batch����������softmaxֵ�����pair��
	std::vector<std::pair<float, int>> pairs;
	for (size_t i = 0; i < v.size(); ++i)
		pairs.push_back(std::make_pair(v[i], i));

	// ��pairs����
	std::partial_sort(pairs.begin(), pairs.begin() + N, pairs.end(), PairCompare);

	// ��pairs������ǰN�����������
	std::vector<int> result;
	for (int i = 0; i < N; ++i)
		result.push_back(pairs[i].second);
	return result;
}
// \! �������
int clsPost(
	float* floatarr, 
	std::vector<std::vector<ClassifyResult>>& outputs, 
	const int batch, 
	const int num_class
)
{
	// 1. Top K
	int N = 3;
	auto K = N > num_class ? num_class : N;

	// 2. softmax
	std::vector<float> output_buffer;
	for (int b = 0; b < batch; b++) {
		// 2.1 ��һ��batch��sum�� ����ÿ�����exp��ֵ��ŵ�output_buffer��
		float sum{ 0.0f };
		for (int i = 0; i < num_class; i++) {
			output_buffer.push_back(exp(floatarr[b * num_class + i]));
			sum += output_buffer[b * num_class + i];
		}
		// 2.2 ��softmax��output���һ��ͼƬ�������������Ŷ�
		std::vector<float> output;
		for (int j = 0; j < num_class; j++) {
			output.push_back(output_buffer[b * num_class + j] / sum);
		}
		// 2.3 ��Top K���±�ͷ�ֵ��output topk��index ����maxN��
		std::vector<int> maxN = Argmax(output, K);
		std::vector<ClassifyResult> classifyResults;
		for (int i = 0; i < K; ++i)
		{
			int idx = maxN[i];
			classifyResults.push_back(std::make_pair(idx, output[idx]));
		}
		outputs.push_back(classifyResults);
	}

	return FF_OK;
}


// \! �쳣������
int anomalyPost(
	float* floatarr,// onnxruntime ����Ľ��
	std::vector<cv::Mat>& outputs,// �洢����Ľ��
	const int output_batch,// output��batchsize
	const int output_height,//output�ĸ�
	const int output_width//output�Ŀ�
)
{

	for (int i = 0; i < output_batch; i++)
	{
		cv::Mat res = cv::Mat(output_height, output_width, CV_32FC1, floatarr + output_height * output_width * i).clone();
		outputs.push_back(res);
	}
	return FF_OK;
}

// \! �ָ����
int segPost(
	float* floatarr,// onnxruntime ����Ľ��
	std::vector<cv::Mat>& outputs,// �洢����Ľ��
	const int output_batch,// output��batchsize
	const int output_height,//output�ĸ�
	const int output_width//output�Ŀ�
)
{
	for (int i = 0; i < output_batch; i++){
		cv::Mat res = cv::Mat(output_height, output_width, CV_32FC1, floatarr + output_height * output_width * i).clone();
		outputs.push_back(res);
	}
	return FF_OK;
}


//int segPostOutput(const samplesCommon::BufferManager & buffers, std::vector<cv::Mat>& out_masks, const int numSamples,
//	const Params& params, const nvinfer1::Dims outputDims, bool verbose, const std::string outputName)
//{
//	out_masks.clear();
//	float* output = static_cast<float*>(buffers.getHostBuffer(outputName));
//	int32_t num_class, height, width;
//
//	if (verbose) // �������ͼ
//	{
//		num_class = outputDims.d[1];
//		height = outputDims.d[2];
//		width = outputDims.d[3];
//		for (int i = 0; i < numSamples; i++)
//		{
//			std::vector<cv::Mat> n_channels;
//			//cv::Mat res = cv::Mat(height, width, CV_32FC(num_class), output + height * width * num_class * i).clone();
//			cv::Mat res;
//			for (int j = 0; j < num_class; j++) {
//				cv::Mat tmp = cv::Mat(height, width, CV_32F, output + height * width * j + height * width * i * num_class).clone();
//				n_channels.push_back(tmp);
//			}
//			cv::merge(n_channels, res);
//			out_masks.push_back(res);
//		}
//	}
//	else
//	{
//		if (outputDims.nbDims == 3)  // onnx�����������������ת���������Խ�࣬���ַ�ʽԽ����Ч������
//		{
//			num_class = 0;  // ������ͼ�������Ѿ�û��num_class��num_class������ֵ������
//			height = outputDims.d[1];
//			width = outputDims.d[2];
//
//			for (int i = 0; i < numSamples; i++)
//			{
//				cv::Mat res = cv::Mat(height, width, CV_32F, output + height * width * i).clone();
//				res.convertTo(res, CV_8UC1);
//				out_masks.push_back(res);
//			}
//		}
//		else  // onnx�����Ǹ���ֵ����Ҫ�Լ�����ת����Ч�ʵͣ�Ϊ�˼���֮ǰ�Ĳű����˶δ����֧�����ڽ�����֧�֡�
//		{
//			num_class = outputDims.d[1];
//			height = outputDims.d[2];
//			width = outputDims.d[3];
//			if (num_class == 2)
//			{  // ֻ��Ҫ��ǰ���������ָ��ʱ��������ʽ�Ĵ���Ч�ʸߡ��ɶ���ǿ
//				for (int n = 0; n < numSamples; n++)
//				{
//					// �������h*w�ľ��󡣵�һ����bg�ĸ��ʣ��ڶ�����crack�ĸ��ʡ�����ĸ�����ӵ���1
//					// ��bg����0.5Ϊ��ֵ��ֵ����С��0.5��Ϊcrack
//					cv::Mat tmp = cv::Mat(height, width, CV_32F, output + height * width * (2 * n)).clone();
//					cv::threshold(tmp, tmp, 0.5f, 1.f, cv::THRESH_BINARY_INV);
//					tmp.convertTo(tmp, CV_8UC1);
//					out_masks.push_back(tmp);
//				}
//			}
//			else
//			{	// ���������2�����
//				for (int n = 0; n < numSamples; n++)
//				{
//					std::vector<cv::Mat> n_channels;
//					//cv::Mat res = cv::Mat(height, width, CV_32FC(num_class), output + height * width * num_class * i).clone();
//					cv::Mat res;
//					for (int j = 0; j < num_class; j++) {
//						cv::Mat tmp = cv::Mat(height, width, CV_32F, output + height * width * j + height * width * n * num_class).clone();
//						n_channels.push_back(tmp);
//					}
//					cv::merge(n_channels, res);
//					res.convertTo(res, CV_8UC(num_class));
//
//					cv::Mat tmp = cv::Mat::zeros(height, width, CV_8U);
//					for (int h = 0; h < height; ++h) {
//						for (int w = 0; w < width; ++w) {
//							//uchar *p = res.ptr(h, w); // prob of a point
//							uchar *p = res.ptr(h, w); // prob of a point
//							tmp.at<uchar>(h, w) = (uchar)std::distance(p, std::max_element(p, p + num_class));
//						}
//					}
//					// todo Ƕ��ѭ��Ч�ʼ��͡�����
//					//cv::Mat tmp = cv::Mat::zeros(height, width, CV_32F);
//					//for (int row = 0; row < height; row++) {
//					//	float* dataP = tmp.ptr<float>(row);
//					//	for (int col = 0; col < width; col++) {
//					//		std::vector<float> prop(num_class, 0.f);
//					//		for (int indexC = 0; indexC < num_class; indexC++) {
//					//			prop[indexC] = output[n * num_class * height * width + indexC * height * width + row * height + col];
//					//		}
//					//		size_t maxIndex = std::distance(prop.begin(), std::max_element(prop.begin(), prop.end()));
//					//		//size_t maxIndex = argmax(prop.begin(), prop.end());
//					//		dataP[col] = float(maxIndex);
//					//	}
//					//}
//					//tmp.convertTo(tmp, CV_8UC1);
//
//					out_masks.push_back(tmp);
//				}
//			}
//		}
//	}
//
//	return LY_OK;
//}
//

