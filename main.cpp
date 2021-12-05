#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdint.h>

#include <iostream>
#include <vector>

#include "stb_image.h"
#include "stb_image_write.h"

struct Color {
    uint8_t r, g, b;

    int dist(const Color &color) const {
        int dr = abs(r - (int)color.r);
        int dg = abs(g - (int)color.g);
        int db = abs(b - (int)color.b);
        return dr + dg + db;
    }
};

using std::vector;
using ImageArray = vector<vector<Color>>;
using ImageData = uint8_t *;

const int CHANNELS = 3;

ImageArray toImageArray(const ImageData &imageData, const int width, const int height) {
    vector<vector<Color>> imageArray(height, vector<Color>(width));
    int ind = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            imageArray[i][j].r = imageData[ind++];
            imageArray[i][j].g = imageData[ind++];
            imageArray[i][j].b = imageData[ind++];
        }
    }
    return imageArray;
}

ImageData toImageData(const ImageArray &imageArray) {
    const int height = imageArray.size(), width = imageArray[0].size();
    uint8_t *imageData = (uint8_t *)malloc(height * width * CHANNELS * sizeof(uint8_t));

    int ind = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            imageData[ind++] = imageArray[i][j].r;
            imageData[ind++] = imageArray[i][j].g;
            imageData[ind++] = imageArray[i][j].b;
        }
    }
    return imageData;
}

ImageArray kMeans(const ImageArray &imageArray, int k, int iterations) {
    const int height = imageArray.size(), width = imageArray[0].size();
    vector<vector<int>> clusterId(height, vector<int>(width, 0));  // -1 is for not assigned
    vector<Color> clusterColor = {{0, 0, 0}};                      // color of kth cluster

    // iterative?
    for (int it = 0; it < iterations; it++) {
        printf("it: %d colors: %d\n", it, (int)clusterColor.size());

        // choose closest cluster
        vector<vector<int>> sum(clusterColor.size(), vector<int>(3));  // rgb sum
        vector<int> count(clusterColor.size());

        // find the worst-represented cell
        int worst = -1e9;
        int worstI = -1, worstJ = -1;

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                int id = -1;
                int dist = 1e9;

                for (int c = 0; c < clusterColor.size(); c++) {
                    int curDist = imageArray[i][j].dist(clusterColor[c]);
                    if (curDist < dist) {
                        dist = curDist;
                        id = c;
                    }
                }
                assert(id != -1);

                // dist is the closest dist
                if (dist > worst) {
                    worst = dist;
                    worstI = i;
                    worstJ = j;
                }

                clusterId[i][j] = id;
                sum[id][0] += imageArray[i][j].r;
                sum[id][1] += imageArray[i][j].g;
                sum[id][2] += imageArray[i][j].b;
                count[id]++;
            }
        }

        // update cluster
        for (int c = 0; c < clusterColor.size(); c++) {
            assert(count[c] > 0);
            clusterColor[c].r = round(sum[c][0] / (double)count[c]);
            clusterColor[c].g = round(sum[c][1] / (double)count[c]);
            clusterColor[c].b = round(sum[c][2] / (double)count[c]);
        }

        if (clusterColor.size() < k) {
            // if there are not enough colors, add a new color!
            // find the color that is the worst

            clusterColor.push_back(imageArray[worstI][worstJ]);
        }
    }

    auto result = imageArray;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            result[i][j] = clusterColor[clusterId[i][j]];
        }
    }
    return result;
}

int main() {
    // Reading image
    printf("Reading image...\n");
    int width, height, channels;
    ImageData inputImageData = stbi_load("img/interfac.jpg", &width, &height, &channels, CHANNELS);
    if (!inputImageData) {
        throw std::invalid_argument("Error loading image");
    }
    assert(channels == CHANNELS);
    auto inputImageArray = toImageArray(inputImageData, width, height);
    stbi_image_free(inputImageData);

    printf("Processing image...\n");
    int colors = 9;
    auto outputImageArray = kMeans(inputImageArray, colors, colors + 10);

    // Writing image
    printf("Writing image...\n");
    auto outputImageData = toImageData(outputImageArray);
    stbi_write_png("output.jpg", width, height, 3, outputImageData, width * 3);
    free(outputImageData);
    return 0;
}