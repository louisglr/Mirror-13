#pragma once

#include <vector>
#include <cmath>
#include <juce_core/juce_core.h>

// Cirkulær mono-buffer, delt mellem alle stemmer (lead + harmonier) så de
// alle læser fra samme friske input uden at duplikere hukommelse.
class VoiceBuffer
{
public:
    void prepare(double sampleRate, float seconds = 1.0f)
    {
        size = juce::jmax(1024, (int) (sampleRate * (double) seconds));
        buf.assign((size_t) size, 0.0f);
        writeHead = 0;
    }

    void write(float x)
    {
        buf[(size_t) wrapIndex(writeHead)] = x;
        ++writeHead;
    }

    float readInterpolated(double absPos) const
    {
        long long i0 = (long long) std::floor(absPos);
        float frac = (float) (absPos - (double) i0);

        // Four-point Hermite interpolation is noticeably cleaner than linear
        // interpolation when a voice is read at a different speed.  It costs
        // very little here, but removes much of the brittle high-frequency
        // texture that becomes obvious in stacked vocal harmonies.
        const float y0 = buf[(size_t) wrapIndex(i0 - 1)];
        const float y1 = buf[(size_t) wrapIndex(i0)];
        const float y2 = buf[(size_t) wrapIndex(i0 + 1)];
        const float y3 = buf[(size_t) wrapIndex(i0 + 2)];
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * frac + c2) * frac + c1) * frac + y1;
    }

    // Find a nearby rising zero crossing to use as a lightweight pitch mark.
    // The search is only performed when a grain is renewed, never for every
    // sample, so it is suitable for the audio thread at these small radii.
    double findNearestRisingZeroCrossing(double absPos, int searchRadius) const
    {
        const long long centre = (long long) std::llround(absPos);
        const long long start = centre - juce::jmax(1, searchRadius);
        const long long end = centre + juce::jmax(1, searchRadius);
        double bestPosition = absPos;
        double bestDistance = (double) searchRadius + 1.0;

        for (long long i = start; i <= end; ++i)
        {
            const float previous = buf[(size_t) wrapIndex(i - 1)];
            const float current = buf[(size_t) wrapIndex(i)];
            if (previous <= 0.0f && current > 0.0f)
            {
                const double crossing = (double) i - (double) previous / (double) (current - previous);
                const double distance = std::abs(crossing - absPos);
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    bestPosition = crossing;
                }
            }
        }
        return bestPosition;
    }

    long long getWriteHead() const { return writeHead; }
    int getSize() const { return size; }

private:
    int wrapIndex(long long i) const
    {
        long long m = i % (long long) size;
        if (m < 0) m += size;
        return (int) m;
    }

    std::vector<float> buf;
    long long writeHead = 0;
    int size = 0;
};
