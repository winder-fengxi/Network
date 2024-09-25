#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <filesystem>

using namespace cv;
using namespace std;

// 将图片转换为 bin 文件
void imageToBin(const string& image_path, const string& bin_path) {
    Mat image = imread(image_path, IMREAD_UNCHANGED);
    if (image.empty()) {
        cerr << "Error: Could not load image " << image_path << endl;
        return;
    }

    ofstream output_file(bin_path, ios::out | ios::binary);
    if (!output_file) {
        cerr << "Error: Could not open file " << bin_path << endl;
        return;
    }

    int width = image.cols;
    int height = image.rows;
    int channels = image.channels();
    output_file.write(reinterpret_cast<const char*>(&width), sizeof(width));
    output_file.write(reinterpret_cast<const char*>(&height), sizeof(height));
    output_file.write(reinterpret_cast<const char*>(&channels), sizeof(channels));
    output_file.write(reinterpret_cast<const char*>(image.data), image.total() * image.elemSize());

    output_file.close();
    cout << "Image successfully written to " << bin_path << endl;
}

// 生成一系列 RGB 图像
void generateImages(const vector<char>& data, const string& output_dir, int num_images) {
    int data_index = 0;
    int data_size = data.size();

    for (int i = 0; i < num_images; ++i) {
        Mat image(108, 108, CV_8UC3);
        for (int y = 0; y < image.rows; ++y) {
            for (int x = 0; x < image.cols; ++x) {
                Vec3b& color = image.at<Vec3b>(y, x);
                if (data_index < data_size) {
                    color[0] = static_cast<unsigned char>(data[data_index++]); // B
                    color[1] = static_cast<unsigned char>(data[data_index++]); // G
                    color[2] = static_cast<unsigned char>(data[data_index++]); // R
                }
                else {
                    color = Vec3b(0, 0, 0); // 如果数据不足，填充黑色
                }
            }
        }

        string filename = output_dir + "/frame_" + to_string(i) + ".png";
        imwrite(filename, image);
    }
    cout << "RGB images generated successfully." << endl;
}

// 使用 FFMPEG 将图像编码为视频
void encodeImagesToVideo(const string& input_pattern, const string& output_video) {
    string command = "ffmpeg -framerate 30 -i " + input_pattern + " -c:v libx264 -pix_fmt yuv420p " + output_video;
    int result = system(command.c_str());
    if (result != 0) {
        cerr << "Error: ffmpeg command failed" << endl;
    }
    else {
        cout << "Video successfully written to " << output_video << endl;
    }
}

// 将文件编码为视频
void encode(const string& input_bin, const string& output_video, int video_length) {
    ifstream input_file(input_bin, ios::binary | ios::ate);
    if (!input_file) {
        cerr << "Error: Could not open binary file " << input_bin << endl;
        return;
    }

    streamsize size = input_file.tellg();
    input_file.seekg(0, ios::beg);
    vector<char> buffer(size);

    if (!input_file.read(buffer.data(), size)) {
        cerr << "Error: Could not read binary file" << endl;
        return;
    }

    string output_dir = "frames";
    system(("mkdir -p " + output_dir).c_str());
    generateImages(buffer, output_dir, video_length / 33);  // 假设每帧持续 33ms

    string input_pattern = output_dir + "/frame_%d.png";
    encodeImagesToVideo(input_pattern, output_video);
}

// 从视频中解码为 bin 文件和有效性标记文件
void decode(const string& input_video, const string& output_bin, const string& validity_bin) {
    VideoCapture cap(input_video);
    if (!cap.isOpened()) {
        cerr << "Error: Could not open video file " << input_video << endl;
        return;
    }

    vector<char> decoded_data;
    vector<unsigned char> validity_data; // 用于存储每一位的有效性标记
    Mat frame;

    while (cap.read(frame)) {
        if (frame.empty()) {
            cerr << "Error: Could not read frame from video" << endl;
            continue;
        }

        for (int y = 0; y < frame.rows; ++y) {
            for (int x = 0; x < frame.cols; ++x) {
                Vec3b color = frame.at<Vec3b>(y, x);
                decoded_data.push_back(color[0]); // B
                decoded_data.push_back(color[1]); // G
                decoded_data.push_back(color[2]); // R

                // 假设此处的有效性判断规则是简单的检查数据范围
                if (color[0] != 0 || color[1] != 0 || color[2] != 0) {
                    validity_data.push_back(1); // 1 表示有效
                }
                else {
                    validity_data.push_back(0); // 0 表示无效
                }
            }
        }
    }

    // 将解码后的数据写入 bin 文件
    ofstream output_file(output_bin, ios::out | ios::binary);
    if (!output_file) {
        cerr << "Error: Could not open output file " << output_bin << endl;
        return;
    }
    output_file.write(decoded_data.data(), decoded_data.size());
    output_file.close();
    cout << "Decoding completed. Output written to " << output_bin << endl;

    // 将有效性标记写入 vout 文件
    ofstream validity_file(validity_bin, ios::out | ios::binary);
    if (!validity_file) {
        cerr << "Error: Could not open validity output file " << validity_bin << endl;
        return;
    }
    validity_file.write(reinterpret_cast<const char*>(validity_data.data()), validity_data.size());
    validity_file.close();
    cout << "Validity output written to " << validity_bin << endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <command> [args]" << endl;
        cerr << "Commands: " << endl;
        cerr << "  image2bin <input_image> <output_bin>" << endl;
        cerr << "  encode <input_bin> <output_video> <video_length_ms>" << endl;
        cerr << "  decode <input_video> <output_bin> <validity_bin>" << endl; // 更新命令行参数
        return -1;
    }

    string command = argv[1];

    if (command == "image2bin") {
        if (argc != 4) {
            cerr << "Usage: image2bin <input_image> <output_bin>" << endl;
            return -1;
        }
        imageToBin(argv[2], argv[3]);
    }
    else if (command == "encode") {
        if (argc != 5) {
            cerr << "Usage: encode <input_bin> <output_video> <video_length_ms>" << endl;
            return -1;
        }
        encode(argv[2], argv[3], stoi(argv[4]));
    }
    else if (command == "decode") {
        if (argc != 5) {
            cerr << "Usage: decode <input_video> <output_bin> <validity_bin>" << endl; // 更新命令行参数
            return -1;
        }
        decode(argv[2], argv[3], argv[4]); // 调用解码函数，传入有效性文件名
    }
    else {
        cerr << "Unknown command: " << command << endl;
        return -1;
    }

    return 0;
}
