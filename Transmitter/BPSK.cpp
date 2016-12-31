#include <math.h>
#include "BPSK.h"

BPSK::BPSK(Memory * memory, 
        Module * next, 
        float fs, 
        float fc, 
        int cycles_per_bit,
        int training_bits):
    Module(memory, next),
    fs(fs),
    fc(fc),
    cycles_per_bit(cycles_per_bit),
    training_bits(training_bits)
{
    inc = 2.0*M_PI*fc/fs;
    phase = 0;
    one_eighty = M_PI;
    samples_per_cycle = fs/fc;
    phase_per_bit = cycles_per_bit * 2.0 * M_PI;

    double bw = fc/cycles_per_bit * 2.0;
    MAGENTA;
    printf("Estimated bandwidth: %.3f\n", bw);
    ENDC;
}

class BPSKBlock : public Block
{
public:
    BPSKBlock(Block * bits, 
            int training_bits,
            int cycles_per_bit,
            double fc,
            double fs):
        bits(bits),
        training_bits(training_bits),
        cycles_per_bit(cycles_per_bit),
        fc(fc),
        fs(fs)
    {
        samples_per_cycle = fs/fc;
        len = (size_t) ((training_bits + bits->get_size()) * cycles_per_bit * samples_per_cycle);
        phase_per_bit = cycles_per_bit * 2.0 * M_PI;
        inc = 2.0*M_PI*fc/fs;

        ptr = &value;
        reset();
    }

    void free() {
        printf("freeing BPSKBlock...\n");
        bits->free();
        delete this;
    }

    bool is_free() {
        return bits->is_free();
    }

    size_t get_size() {
        return len;
    }

    void reset() {
        bits->reset();
        bits_iter = bits->get_iterator();

        // this is to get it to hit the first if statement in next right after fresh reset
        phase = phase_per_bit;

        inv = false;
        // this forces it to flip on the first bit
        last_bit = **bits_iter == 0.0 ? 1.0 : 0.0;
        state = TRAIN;
        k = 0;
        n = 0;
        bit = 0.0;
    }

    bool next() 
    {
        if (phase >= phase_per_bit) 
        {
            phase -= phase_per_bit;

            if (k >= training_bits) {
                state = LOAD_BIT;
            }

            switch (state)
            {
            case TRAIN:
                k += 1;
                break;

            case LOAD_BIT:
                bit = **bits_iter;

                if (last_bit != bit) 
                {
                    inv ^= true;
                    last_bit = bit;
                }
                bits->next();
                break;
            }
        }

        if (n < len) 
        {
            value = (float) ((inv ? -1.0 : 1.0) * sin(phase));
            phase += inc;
            n += 1;
            return true;
        }
        else 
        {
            return false;
        }
    }

    float ** get_iterator() {
        return &ptr;
    }

    void print() {
        printf("BPSK Block\n");
        bits->print();
    }

private:
    Block * bits;
    BPSK * bpsk;
    double fc;
    double fs;
    int training_bits;
    int cycles_per_bit;
    double phase_per_bit;
    size_t len;
    double samples_per_cycle;

    float * ptr;
    float value;
    double phase;
    double inc;
    bool inv;
    float last_bit;
    float ** bits_iter;
    enum {TRAIN, LOAD_BIT} state;
    int k,n;
    float bit;
};

Block * BPSK::process(Block * bits)
{
    Block * ret = new BPSKBlock(bits, training_bits, cycles_per_bit, fc, fs);
    return ret;
}

/*
Block * BPSK::process(Block * bits)
{
    static double phase = 0.0;
    static bool inv = false;

    float last_bit = 0.0;
    char errors[][32] =
    {{"No error"},
     {"Allocate error"}};
    int error = 0;
    float ** signal_iter = NULL;
    float ** bits_iter = NULL;
    enum {TRAIN, MODULATE} state;
    state = TRAIN;

    bool quit = false;
    int k;
    float bit;

    size_t len = (size_t)
        ((training_bits + bits->get_size()) * cycles_per_bit * samples_per_cycle);

    Block * signal = memory->allocate(len);

    if (!signal) {
        error = 1;
        goto fail;
    }

    signal_iter = signal->get_iterator();
    bits_iter = bits->get_iterator();

    bits->reset();

    // this forces it to flip on the first bit;
    last_bit = **bits_iter == 0.0 ? 1.0 : 0.0;
    k = 0;

    do
    {
        if (k >= training_bits) {
            state = MODULATE;
        }

        switch (state)
        {
            case TRAIN:
            k += 1;
            break;

            case MODULATE:
            bit = **bits_iter;

            if (last_bit != bit) {
                inv ^= true;
                last_bit = bit;
            }

            if (!bits->next()) {
                quit = true;
            }
            break;
        }

        do 
        {
            **signal_iter = (float) ((inv ? -1.0 : 1.0) * sin(phase));
            phase += inc;
        } while (signal->next() && phase < phase_per_bit); 

        phase -= phase_per_bit;

    } while(!quit);

    bits->free();
    return signal;

fail:
    RED;
    printf("%s\n", errors[error]);
    ENDC;
    bits->free();
    if (signal) {
        signal->free();
    }
    return NULL;
}
*/

const char __name__[] = "BPSK";

const char * BPSK::name()
{
    return __name__;
}
