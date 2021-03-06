///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015, STEREOLABS.
//
// All rights reserved.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////



/**************************************************************************************************
 ** This sample demonstrates how to save depth information provided by the ZED Camera,              **
 ** or by an SVO file, in different image formats (PNG 16-Bits, PFM...).                       **
 **                                                                                                 **
 ***************************************************************************************************/

#define NOMINMAX
//standard includes
#include <iomanip>
#include <signal.h>
#include <iostream>
#include <limits>
#include <thread>

//opencv includes
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/utility.hpp>

//ZED Includes
#include <zed/Camera.hpp>

using namespace std;

typedef struct SaveParamStruct {
	sl::POINT_CLOUD_FORMAT PC_Format;
	sl::DEPTH_FORMAT Depth_Format;
	std::string saveName;
	bool askSavePC;
	bool askSaveDepth;
	bool stop_signal;
} SaveParam;

sl::zed::Camera * zed_ptr;
SaveParam *param;

std::string getFormatNamePC(sl::POINT_CLOUD_FORMAT f) {
	std::string str_;
	switch (f) {
	case sl::XYZ:
		str_ = "XYZ";
		break;
	case sl::PCD:
		str_ = "PCD";
		break;
	case sl::PLY:
		str_ = "PLY";
		break;
	case sl::VTK:
		str_ = "VTK";
		break;
	default:
		break;
	}
	return str_;
}

std::string getFormatNameD(sl::DEPTH_FORMAT f) {
	std::string str_;
	switch (f) {
	case sl::PNG:
		str_ = "PNG";
		break;
	case sl::PFM:
		str_ = "PFM";
		break;
	case sl::PGM:
		str_ = "PGM";
		break;
	default:
		break;
	}
	return str_;
}

//UTILS fct to handle CTRL-C event
#ifdef _WIN32

BOOL CtrlHandler(DWORD fdwCtrlType) {
	switch (fdwCtrlType) {
		//Handle the CTRL-C signal.
	case CTRL_C_EVENT:
		printf("\nQuitting...\n");
		delete zed_ptr;
		exit(0);
	default:
		return FALSE;
	}
}
#else

void nix_exit_handler(int s) {
	printf("\nQuitting...\n");
	delete zed_ptr;
	exit(1);
}
#endif

void saveProcess() {
	while (!param->stop_signal) {

		if (param->askSaveDepth) {

			float max_value = std::numeric_limits<unsigned short int>::max();
			float scale_factor = max_value / zed_ptr->getDepthClampValue();

			std::cout << "Saving Depth Map " << param->saveName << " in " << getFormatNameD(param->Depth_Format) << " ..." << flush;
			sl::writeDepthAs(zed_ptr, param->Depth_Format, param->saveName, scale_factor);
			std::cout << "done" << endl;
			param->askSaveDepth = false;
		}

		if (param->askSavePC) {
			std::cout << "Saving Point Cloud " << param->saveName << " in " << getFormatNamePC(param->PC_Format) << " ..." << flush;
			sl::writePointCloudAs(zed_ptr, param->PC_Format, param->saveName, true, false);
			std::cout << "done" << endl;
			param->askSavePC = false;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

//Main function
int main(int argc, char **argv) {

	sl::zed::Camera* zed;
	int nbFrames = 0;
	sl::zed::MODE depth_mode = sl::zed::MODE::PERFORMANCE;

	const cv::String keys =
		"{help h usage ? || print this message}"
		"{filename||SVO filename}"
		"{resolution|2|ZED Camera resolution, ENUM; 0: HD2K   1: HD1080   2: HD720   3: VGA}"
		"{mode|1|Disparity Map mode, ENUM; 1: PERFORMANCE  2: MEDIUM   3: QUALITY}"
		"{path|./|Output path (can include output filename prefix)}"
		"{device|-1|CUDA device}";

	cv::CommandLineParser parser(argc, argv, keys);
	parser.about("Sample from ZED SDK " + sl::zed::Camera::getSDKVersion());
	if (parser.has("help")){
		parser.printMessage();
		cout << " Example : ZED Save depth.exe --resolution=3 --mode=2" << endl;
		return 0;
	}

	string filename = parser.get<cv::String>("filename");
	if (filename.empty()){
		cout << "Stream		> LIVE" << endl;
		int resolution = parser.get<int>("resolution");
		if ((resolution < (int)sl::zed::ZEDResolution_mode::HD2K) || (resolution >(int)sl::zed::ZEDResolution_mode::VGA)){
			cout << "resolution " << resolution << " is not available." << endl;
			parser.printMessage();
			return 0;
		}

		cout << "Resolution	> " << flush;
		switch (resolution) {
		case 0: cout << "HD2K" << endl;
			break;
		case 1: cout << "HD1080" << endl;
			break;
		case 2: cout << "HD720" << endl;
			break;
		case 3: cout << "VGA" << endl;
			break;
			break;
		}
		zed = new sl::zed::Camera(static_cast<sl::zed::ZEDResolution_mode> (resolution));
	}
	else{
		cout << "Stream		> SVO :" << filename << endl;
		zed = new sl::zed::Camera(filename);
		nbFrames = zed->getSVONumberOfFrames();
	}

	int mode = parser.get<int>("mode");
	if ((mode < (int)sl::zed::MODE::PERFORMANCE) || (mode >(int)sl::zed::MODE::QUALITY)){
		cout << "mode " << mode << " is not available." << endl;
		parser.printMessage();
		return 0;
	}

	depth_mode = static_cast<sl::zed::MODE>(mode);
	cout << "Mode		> " << sl::zed::mode2str(depth_mode) << endl;

	string path = parser.get<std::string>("path");
	int device = parser.get<int>("device");

	if (!parser.check()){
		parser.printErrors();
		return 0;
	}

	string prefixPC = "PC_"; //Default output file prefix
	string prefixDepth = "Depth_"; //Default output file prefix

	sl::zed::InitParams parameters;
	parameters.mode = depth_mode;
	parameters.unit = sl::zed::UNIT::MILLIMETER;
	parameters.verbose = 1;
	parameters.device = device;

	sl::zed::ERRCODE err = zed->init(parameters);
	zed_ptr = zed;
	//ERRCODE display
	cout << errcode2str(err) << endl;

	//Quit if an error occurred
	if (err != sl::zed::ERRCODE::SUCCESS) {
		delete zed;
		return 1;
	}

	//CTRL-C (= kill signal) handler
#ifdef _WIN32
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
#else // unix
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = nix_exit_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
#endif

	char key = ' '; // key pressed
	int width = zed_ptr->getImageSize().width;
	int height = zed_ptr->getImageSize().height;
	cv::Size size(width, height); // image size
	cv::Mat depthDisplay(size, CV_8UC1); // normalized depth to display

	bool printHelp = false;
	std::string helpString = "[d] save Depth, [P] Save Point Cloud, [m] change format PC, [n] change format Depth, [q] quit";

	int depth_clamp = 5000;
	zed_ptr->setDepthClampValue(depth_clamp);

	int mode_PC = 0;
	int mode_Depth = 0;

	param = new SaveParam();
	param->askSavePC = false;
	param->askSaveDepth = false;
	param->stop_signal = false;
	param->PC_Format = static_cast<sl::POINT_CLOUD_FORMAT> (mode_PC);
	param->Depth_Format = static_cast<sl::DEPTH_FORMAT> (mode_Depth);

	std::thread grab_thread(saveProcess);

	bool quit_ = false;

	std::cout << " Press 'p' to save Point Cloud" << std::endl;
	std::cout << " Press 'd' to save Depth image" << std::endl;

	std::cout << " Press 'm' to switch Point Cloud format" << std::endl;
	std::cout << " Press 'n' to switch Depth format" << std::endl;

	std::cout << " Press 'q' to exit" << std::endl;
	int count = 0;
	while (!quit_ && (zed_ptr->getSVOPosition() <= nbFrames)) {

		zed_ptr->grab();
		slMat2cvMat(zed_ptr->normalizeMeasure(sl::zed::MEASURE::DEPTH)).copyTo(depthDisplay);

		if (printHelp) // write help text on the image if needed
			cv::putText(depthDisplay, helpString, cv::Point(20, 20), CV_FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(111, 111, 111, 255), 2);

		cv::imshow("Depth", depthDisplay);
		key = cv::waitKey(5);

		switch (key) {
		case 'p':
		case 'P':
			param->saveName = path + prefixPC + to_string(count);
			param->askSavePC = true;
			break;

		case 'd':
		case 'D':
			param->saveName = path + prefixDepth + to_string(count);
			param->askSaveDepth = true;
			break;

		case 'm':
		case 'M':
		{
			mode_PC++;
			param->PC_Format = static_cast<sl::POINT_CLOUD_FORMAT> (mode_PC % 4);
			std::cout << "Format Point Coud " << getFormatNamePC(param->PC_Format) << std::endl;
		}
		break;

		case 'n':
		case 'N':
		{
			mode_Depth++;
			param->Depth_Format = static_cast<sl::DEPTH_FORMAT> (mode_Depth % 3);
			std::cout << "Format Depth " << getFormatNameD(param->Depth_Format) << std::endl;
		}
		break;

		case 'h': // print help
		case 'H':
			printHelp = !printHelp;
			cout << helpString << endl;
			break;

		case 'q': // QUIT
		case 'Q':
		case 27:
			quit_ = true;
			break;
		}
		count++;
	}

	param->stop_signal = true;
	grab_thread.join();

	delete zed_ptr;

	return 0;
}
