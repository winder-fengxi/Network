#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

using namespace cv;
using namespace std;

// 计算CRC（简单实现，可以替换为更复杂的CRC算法）
uint32_t calculateCRC(const vector<char>& data) {
    uint32_t crc = 0;
    for (char byte : data) {
        crc += static_cast<uint8_t>(byte);
    }
    return crc;
}

// 编码帧
vector<char> createFrame(const vector<char>& payload, uint8_t destAddr, uint8_t srcAddr) {
    vector<char> frame;

    // 帧首定界符
    frame.push_back(static_cast<char>(0xCA));
    frame.push_back(static_cast<char>(destAddr));
    frame.push_back(static_cast<char>(srcAddr));

    if (payload.size() > UINT16_MAX) {
        cerr << "Error: Payload size exceeds maximum value for uint16_t" << endl;
        return frame;
    }

    uint16_t length = static_cast<uint16_t>(payload.size());
    frame.push_back(static_cast<char>(length >> 8)); // 高字节
    frame.push_back(static_cast<char>(length & 0xFF)); // 低字节

    frame.insert(frame.end(), payload.begin(), payload.end());

    // 计算CRC
    uint32_t crc = calculateCRC(frame);
    frame.push_back(static_cast<char>((crc >> 24) & 0xFF)); // CRC 高字节
    frame.push_back(static_cast<char>((crc >> 16) & 0xFF)); // CRC 中高字节
    frame.push_back(static_cast<char>((crc >> 8) & 0xFF)); // CRC 中低字节
    frame.push_back(static_cast<char>(crc & 0xFF)); // CRC 低字节

    // 帧尾定界符
    frame.push_back(static_cast<char>(0xCA));

    return frame;
}

void generateImages(const vector<char>& videoData, const string& outputDir, int frameInterval) {
    size_t frameSize = 108 * 108 * 3; // 每帧图像的字节数
    size_t frameCount = videoData.size() / frameSize; // 计算帧数

    if (frameCount == 0) {
        cerr << "Error: No frames to generate. Check the size of videoData." << endl;
        cerr << "videoData size: " << videoData.size() << " bytes, required size for one frame: " << frameSize << " bytes." << endl;
        return;
    }

    // 确保输出目录存在
    if (!filesystem::exists(outputDir)) {
        filesystem::create_directories(outputDir);
    }

    for (size_t i = 0; i < frameCount; ++i) {
        Mat img(108, 108, CV_8UC3, Scalar(0, 0, 0)); // 生成 108x108 的图像
        for (int y = 0; y < 108; ++y) {
            for (int x = 0; x < 108; ++x) {
                size_t index = (i * frameSize) + (y * 108 + x) * 3;
                if (index + 2 < videoData.size()) {
                    img.at<Vec3b>(y, x)[0] = videoData[index];     // B
                    img.at<Vec3b>(y, x)[1] = videoData[index + 1]; // G
                    img.at<Vec3b>(y, x)[2] = videoData[index + 2]; // R
                }
            }
        }

        // 保存图像
        stringstream ss;
        ss << outputDir << "/frame_" << setw(4) << setfill('0') << i << ".png";
        imwrite(ss.str(), img);
    }
}

// 将图像编码为视频
void encodeImagesToVideo(const string& inputPattern, const string& outputVideo) {
    string cmd = "ffmpeg -framerate 30 -i " + inputPattern + " -c:v libx264 -pix_fmt yuv420p " + outputVideo;
    system(cmd.c_str());
}

void encode(const string& input_dir, uint16_t mtu, const string& output_video, int video_length) {
    vector<string> bin_files;

    // 遍历目录获取所有.bin文件
    for (const auto& entry : filesystem::directory_iterator(input_dir)) {
        if (entry.path().extension() == ".bin") {
            bin_files.push_back(entry.path().string());
        }
    }

    if (bin_files.empty()) {
        cerr << "Error: No .bin files found in the input directory." << endl;
        return;
    }

    // 存储所有帧数据
    vector<char> videoData;
    for (const auto& bin_file : bin_files) {
        ifstream input_file(bin_file, ios::binary);
        if (!input_file) {
            cerr << "Error: Could not open binary file " << bin_file << endl;
            continue;
        }

        vector<char> payload((istreambuf_iterator<char>(input_file)), (istreambuf_iterator<char>()));
        input_file.close();

        if (payload.empty()) {
            cerr << "Warning: Binary file " << bin_file << " is empty." << endl;
            continue;
        }

        // 将载荷分成适当大小的帧
        size_t offset = 0;
        while (offset < payload.size()) {
            size_t chunkSize = min(static_cast<size_t>(mtu - 6), payload.size() - offset); // 6是首尾定界符和地址的字节数
            vector<char> chunk(payload.begin() + offset, payload.begin() + offset + chunkSize);
            auto frame = createFrame(chunk, 0x01, 0x02); // 示例地址
            videoData.insert(videoData.end(), frame.begin(), frame.end());
            offset += chunkSize;
        }
    }

    if (videoData.empty()) {
        cerr << "Error: No video data generated from the .bin files." << endl;
        return;
    }

    // 将视频数据写入视频
    // 这里我们需要创建图像并将其编码为视频（使用OpenCV和FFmpeg）
    string output_dir = "frames";
    if (!filesystem::exists(output_dir)) {
        filesystem::create_directories(output_dir);
    }

    // 生成RGB图像并编码为视频
    generateImages(videoData, output_dir, video_length / 33);  // 假设每帧持续 33ms
    string input_pattern = output_dir + "/frame_%04d.png";
    encodeImagesToVideo(input_pattern, output_video);
}

// 解码器
void decode(const string& input_video, const string& output_dir) {
    VideoCapture cap(input_video);
    if (!cap.isOpened()) {
        cerr << "Error: Could not open video file " << input_video << endl;
        return;
    }

    vector<char> decoded_data;
    Mat frame;

    while (cap.read(frame)) {
        if (frame.empty()) {
            cerr << "Error: Could not read frame from video" << endl;
            continue;
        }

        // 处理帧并提取数据
        for (int y = 0; y < frame.rows; ++y) {
            for (int x = 0; x < frame.cols; ++x) {
                Vec3b color = frame.at<Vec3b>(y, x);
                decoded_data.push_back(color[0]);
                decoded_data.push_back(color[1]);
                decoded_data.push_back(color[2]);
            }
        }
    }

    // 确保输出目录存在
    if (!filesystem::exists(output_dir)) {
        filesystem::create_directories(output_dir);
    }

    // 解析帧并输出.bin和.val文件
    size_t frameSize = 108 * 108 * 3; // 每帧图像的字节数
    size_t frameCount = decoded_data.size() / frameSize; // 计算帧数

    for (size_t i = 0; i < frameCount; ++i) {
        stringstream binFileName, valFileName;
        binFileName << output_dir << "/frame_" << setw(4) << setfill('0') << i << ".bin";
        valFileName << output_dir << "/frame_" << setw(4) << setfill('0') << i << ".val";

        ofstream binFile(binFileName.str(), ios::binary);
        if (binFile) {
            binFile.write(&decoded_data[i * frameSize], frameSize);
            binFile.close();
        }

        ofstream valFile(valFileName.str(), ios::binary);
        if (valFile) {
            // 示例有效性标记：假设每个字节都有效
            vector<char> validity(frameSize, 0x01);
            valFile.write(validity.data(), validity.size());
            valFile.close();
        }
    }

    cout << "Decoding completed." << endl;
}



int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <command> [args]" << endl;
        cerr << "Commands: " << endl;
        cerr << "  encode <input_dir> <mtu> <output_video> <video_length_ms>" << endl;
        cerr << "  decode <input_video> <output_dir>" << endl;
        return -1;
    }

    string command = argv[1];

    if (command == "encode") {
        if (argc != 6) {
            cerr << "Usage: encode <input_dir> <mtu> <output_video> <video_length_ms>" << endl;
            return -1;
        }
        encode(argv[2], stoi(argv[3]), argv[4], stoi(argv[5]));
    }
    else if (command == "decode") {
        if (argc != 4) {
            cerr << "Usage: decode <input_video> <output_dir>" << endl;
            return -1;
        }
        decode(argv[2], argv[3]);
    }
    else {
        cerr << "Unknown command: " << command << endl;
        return -1;
    }

    return 0;
}
