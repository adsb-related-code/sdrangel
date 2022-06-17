#include "M17Modulator.h"

namespace mobilinkd
{

/*
	sync_word_t preamble_sync{{+3, -3, +3, -3, +3, -3, +3, -3}, 29.f};
	sync_word_t lsf_sync{     {+3, +3, +3, +3, -3, -3, +3, -3}, 32.f, -31.f};	// LSF or STREAM (inverted)
	sync_word_t packet_sync{  {+3, -3, +3, +3, -3, -3, -3, -3}, 31.f, -31.f};	// PACKET or BERT (inverted)
*/

const std::array<uint8_t, 2> M17Modulator::SYNC_WORD        = {0x32, 0x43}; // 00 11  00 10  01 00  00 11 | +1 -3  +1 -1  +3 +1  +1 -3 ????
const std::array<uint8_t, 2> M17Modulator::LSF_SYNC_WORD    = {0x55, 0xF7}; // 01 01  01 01  11 11  01 11 | +3 +3  +3 +3  -3 -3  +3 -3
const std::array<uint8_t, 2> M17Modulator::STREAM_SYNC_WORD = {0xFF, 0x5D}; // 11 11  11 11  01 01  11 01 | -3 -3  -3 -3  +3 +3  -3 +3
const std::array<uint8_t, 2> M17Modulator::PACKET_SYNC_WORD = {0x75, 0xFF}; // 01 11  01 01  11 11  11 11 | +3 -3  +3 +3  -3 -3  -3 -3
const std::array<uint8_t, 2> M17Modulator::BERT_SYNC_WORD   = {0xDF, 0x55}; // 11 01  11 11  01 01  01 01 | -3 +3  -3 -3  +3 +3  +3 +3
const std::array<uint8_t, 2> M17Modulator::EOT_SYNC         = {0x55, 0x5D}; // ????

// Generated using scikit-commpy
// const std::array<float, 150> M17Modulator::rrc_taps = std::array<float, 150>{
//      0.0029364388513841593,  0.0031468394550958484,  0.002699564567597445,   0.001661182944400927,   0.00023319405581230247, // 5
//     -0.0012851320781224025, -0.0025577136087664687, -0.0032843366522956313, -0.0032697038088887226, -0.0024733964729590865,  // 10
//     -0.0010285696910973807,  0.0007766690889758685,  0.002553421969211845,   0.0038920145144327816,  0.004451886520053017,   // 15
//      0.00404219185231544,    0.002674727068399207,   0.0005756567993179152, -0.0018493784971116507, -0.004092346891623224,   // 20
//     -0.005648131453822014,  -0.006126925416243605,  -0.005349511529163396,  -0.003403189203405097,  -0.0006430502751187517,  // 25
//      0.002365929161655135,   0.004957956568090113,   0.006506845894531803,   0.006569574194782443,   0.0050017573119839134,  // 30
//      0.002017321931508163,  -0.0018256054303579805, -0.00571615173291049,   -0.008746639552588416,  -0.010105075751866371,   // 35
//     -0.009265784007800534,  -0.006136551625729697,  -0.001125978562075172,   0.004891777252042491,   0.01071805138282269,    // 40
//      0.01505751553351295,    0.01679337935001369,    0.015256245142156299,   0.01042830577908502,    0.003031522725559901,   // 45
//     -0.0055333532968188165, -0.013403099825723372,  -0.018598682349642525,  -0.01944761739590459,   -0.015005271935951746,   // 50
//     -0.0053887880354343935,  0.008056525910253532,   0.022816244158307273,   0.035513467692208076,   0.04244131815783876,    // 55
//      0.04025481153629372,    0.02671818654865632,    0.0013810216516704976, -0.03394615682795165,   -0.07502635967975885,    // 60
//     -0.11540977897637611,   -0.14703962203941534,   -0.16119995609538576,   -0.14969512896336504,   -0.10610329539459686,    // 65
//     -0.026921412469634916,   0.08757875030779196,    0.23293327870303457,    0.4006012210123992,     0.5786324696325503,     // 70
//      0.7528286479934068,     0.908262741447522,      1.0309661131633199,     1.1095611856548013,     1.1366197723675815,     // 75
//      1.1095611856548013,     1.0309661131633199,     0.908262741447522,      0.7528286479934068,     0.5786324696325503,     // 80
//      0.4006012210123992,     0.23293327870303457,    0.08757875030779196,   -0.026921412469634916,  -0.10610329539459686,    // 85
//     -0.14969512896336504,   -0.16119995609538576,   -0.14703962203941534,   -0.11540977897637611,   -0.07502635967975885,    // 90
//     -0.03394615682795165,    0.0013810216516704976,  0.02671818654865632,    0.04025481153629372,    0.04244131815783876,    // 95
//      0.035513467692208076,   0.022816244158307273,   0.008056525910253532,  -0.0053887880354343935, -0.015005271935951746,   // 100
//     -0.01944761739590459,   -0.018598682349642525,  -0.013403099825723372,  -0.0055333532968188165,  0.003031522725559901,   // 105
//      0.01042830577908502,    0.015256245142156299,   0.01679337935001369,    0.01505751553351295,    0.01071805138282269,    // 110
//      0.004891777252042491,  -0.001125978562075172,  -0.006136551625729697,  -0.009265784007800534,  -0.010105075751866371,   // 115
//     -0.008746639552588416,  -0.00571615173291049,   -0.0018256054303579805,  0.002017321931508163,   0.0050017573119839134,  // 120
//      0.006569574194782443,   0.006506845894531803,   0.004957956568090113,   0.002365929161655135,  -0.0006430502751187517,  // 125
//     -0.003403189203405097,  -0.005349511529163396,  -0.006126925416243605,  -0.005648131453822014,  -0.004092346891623224,   // 130
//     -0.0018493784971116507,  0.0005756567993179152,  0.002674727068399207,   0.00404219185231544,    0.004451886520053017,   // 135
//      0.0038920145144327816,  0.002553421969211845,   0.0007766690889758685, -0.0010285696910973807, -0.0024733964729590865,  // 140
//     -0.0032697038088887226, -0.0032843366522956313, -0.0025577136087664687, -0.0012851320781224025,  0.00023319405581230247, // 145
//      0.001661182944400927,   0.002699564567597445,   0.0031468394550958484,  0.0029364388513841593,  0.0                     // 150
// };

// Generated using scikit-commpy N = 150, aplha = 0.5, Ts = 1/4800 s, Fs = 48000 Hz
/*
    import sys
    import commpy.filters

    N = 150
    alpha=float(sys.argv[1])
    tsymbol = 1/4800
    fs = 48000

    print(f'N={N} alpha={alpha} tsym={tsymbol} fs={fs}')

    t, h = commpy.filters.rrcosfilter(N, alpha, tsymbol, fs)
    line = "    "

    print(len(h))

    for i, coeff in enumerate(h):
        line += f'{coeff:+e}, '
        if (i+1) % 5 == 0:
            print(line)
            line = "    "

    print("")
*/
const std::array<float, 150> M17Modulator::rrc_taps = std::array<float, 150>{
    -8.434122e-04, +3.898184e-04, +1.628891e-03, +2.576674e-03, +2.987740e-03,
    +2.729505e-03, +1.820181e-03, +4.333001e-04, -1.134215e-03, -2.525029e-03,
    -3.402452e-03, -3.531573e-03, -2.841363e-03, -1.451929e-03, +3.417005e-04,
    +2.128236e-03, +3.470212e-03, +4.006361e-03, +3.543024e-03, +2.112321e-03,
    -1.893023e-05, -2.395144e-03, -4.465932e-03, -5.709548e-03, -5.759027e-03,
    -4.501582e-03, -2.125844e-03, +8.982825e-04, +3.907892e-03, +6.188389e-03,
    +7.139194e-03, +6.427125e-03, +4.090469e-03, +5.649447e-04, -3.381677e-03,
    -6.799652e-03, -8.765190e-03, -8.603529e-03, -6.076811e-03, -1.489520e-03,
    +4.321674e-03, +1.012385e-02, +1.453219e-02, +1.631886e-02, +1.472302e-02,
    +9.691259e-03, +1.984723e-03, -6.888307e-03, -1.492227e-02, -2.001531e-02,
    -2.045303e-02, -1.538011e-02, -5.154591e-03, +8.509368e-03, +2.267330e-02,
    +3.359618e-02, +3.740502e-02, +3.091849e-02, +1.248579e-02, -1.731807e-02,
    -5.529141e-02, -9.561492e-02, -1.303248e-01, -1.502279e-01, -1.461577e-01,
    -1.104008e-01, -3.808012e-02, +7.173159e-02, +2.153664e-01, +3.845237e-01,
    +5.668902e-01, +7.473693e-01, +9.097718e-01, +1.038746e+00, +1.121677e+00,
    +1.150282e+00, +1.121677e+00, +1.038746e+00, +9.097718e-01, +7.473693e-01,
    +5.668902e-01, +3.845237e-01, +2.153664e-01, +7.173159e-02, -3.808012e-02,
    -1.104008e-01, -1.461577e-01, -1.502279e-01, -1.303248e-01, -9.561492e-02,
    -5.529141e-02, -1.731807e-02, +1.248579e-02, +3.091849e-02, +3.740502e-02,
    +3.359618e-02, +2.267330e-02, +8.509368e-03, -5.154591e-03, -1.538011e-02,
    -2.045303e-02, -2.001531e-02, -1.492227e-02, -6.888307e-03, +1.984723e-03,
    +9.691259e-03, +1.472302e-02, +1.631886e-02, +1.453219e-02, +1.012385e-02,
    +4.321674e-03, -1.489520e-03, -6.076811e-03, -8.603529e-03, -8.765190e-03,
    -6.799652e-03, -3.381677e-03, +5.649447e-04, +4.090469e-03, +6.427125e-03,
    +7.139194e-03, +6.188389e-03, +3.907892e-03, +8.982825e-04, -2.125844e-03,
    -4.501582e-03, -5.759027e-03, -5.709548e-03, -4.465932e-03, -2.395144e-03,
    -1.893023e-05, +2.112321e-03, +3.543024e-03, +4.006361e-03, +3.470212e-03,
    +2.128236e-03, +3.417005e-04, -1.451929e-03, -2.841363e-03, -3.531573e-03,
    -3.402452e-03, -2.525029e-03, -1.134215e-03, +4.333001e-04, +1.820181e-03,
    +2.729505e-03, +2.987740e-03, +2.576674e-03, +1.628891e-03, +3.898184e-04,
};

} // namespace mobilinkd

