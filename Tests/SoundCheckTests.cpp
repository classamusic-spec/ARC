// Renders every EXCITER x MATERIAL combination (C4, identical settings) to WAV and
// prints the measurements used to voice the materials. Assertions live in the
// exciter / material gates; this test only requires finite, bounded output.

#include "ArcTest.h"
#include "VoiceRig.h"

#include <fstream>

using namespace arctest;

TEST_CASE ("soundcheck", "exciter x material matrix")
{
    const double sr = 48000.0;
    std::ofstream csv (outputDir() + "/soundcheck_matrix.csv");
    csv << "exciter,material,peak,rmsAttack,rmsSustain,centroidHz,t60,fundamentalCents,inharmonicity,partials\n";

    for (int e = 0; e < 4; ++e)
        for (int m = 0; m < 4; ++m)
        {
            auto s = defaultRig();
            s.exciter = static_cast<arc::ExciterType> (e);
            s.material = static_cast<arc::MaterialType> (m);
            const bool sustained = s.exciter == arc::ExciterType::bow || s.exciter == arc::ExciterType::air;
            const double hold = sustained ? 2.0 : 1.5;
            const auto r = renderVoice (s, 60, 0.8f, hold, hold + 3.0, sr);
            REQUIRE (allFinite (r.mono));

            const double f0 = 261.6256;
            const double peak = peakAbs (r.mono);
            const double rmsA = rms (r.mono, 0, static_cast<int> (0.3 * sr));
            const double rmsS = rms (r.mono, static_cast<int> (1.2 * sr), static_cast<int> (0.5 * sr));
            const double cent = spectralCentroid (r.mono, sr, 0, static_cast<int> (0.5 * sr));
            Signal tail (r.mono.begin() + static_cast<long> ((sustained ? hold : 0.0) * sr), r.mono.end());
            const double t60 = measureBroadbandT60 (tail, sr);
            const auto spec = computeSpectrum (r.mono, sr, 1 << 17, sustained ? static_cast<int> (0.5 * sr) : 0,
                                               static_cast<int> (1.0 * sr));
            const double fPeak = peakFrequency (spec, f0 * 0.95, f0 * 1.05);
            const double fine = fPeak > 0 ? refineFrequencyByPhase (r.mono, sr, fPeak) : 0.0;
            const double cents = fine > 0 ? centsBetween (fine, f0) : 0.0;
            const double inh = inharmonicity (spec, f0, 8);
            const auto partials = strongestPartials (spec, 8, 40.0, 16000.0, 15.0);

            const std::string tag = std::string (exciterName (s.exciter)) + "." + materialName (s.material);
            MEASURE (tag + ".peak", peak);
            MEASURE (tag + ".rmsAttack", rmsA);
            MEASURE (tag + ".rmsSustain", rmsS);
            MEASURE (tag + ".centroidHz", cent);
            MEASURE (tag + ".t60", t60);
            MEASURE (tag + ".fundamentalCents", cents);
            MEASURE (tag + ".inharmonicity", inh);
            std::string plist;
            for (auto& p : partials)
            {
                char b[32];
                std::snprintf (b, sizeof (b), "%.2f(%.0f) ", p.freq / f0, p.magDb);
                plist += b;
            }
            std::printf ("    %-24s partials/f0: %s\n", tag.c_str(), plist.c_str());
            csv << exciterName (s.exciter) << "," << materialName (s.material) << "," << peak << "," << rmsA << ","
                << rmsS << "," << cent << "," << t60 << "," << cents << "," << inh << ",\"" << plist << "\"\n";
            writeWav (outputDir() + "/soundcheck_" + tag + ".wav", r.left, r.right, sr);
            CHECK (peak < 1.5);
        }
}
