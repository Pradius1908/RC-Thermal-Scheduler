/*
 * RC-Based Thermal-Aware Scheduler Controller (SAFE VERSION)
 *
 * Course Project – User Space
 *
 * Key properties:
 *  - DOES NOT modify kernel scheduler
 *  - DOES NOT terminate processes
 *  - Uses reversible, rate-limited mitigation
 *  - Uses RC thermal prediction
 *
 * Compile:
 *   gcc rc_thermal_scheduler.c -o rc_sched -lm
 *
 * Run:
 *   sudo ./rc_sched
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <string.h>

/* =======================
   PATHS
   ======================= */
#define TEMP_PATH "/sys/class/thermal/thermal_zone0/temp"
#define FREQ_CUR_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"
#define FREQ_MAX_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"

/* =======================
   RC MODEL PARAMETERS
   ======================= */
#define R_THERMAL   1.0
#define C_THERMAL   10.0
#define T_AMBIENT   30.0
#define DT          1.0

/* =======================
   HYSTERESIS LIMITS
   ======================= */
#define T_HIGH      75.0
#define T_LOW       70.0
#define T_CRITICAL  85.0

/* =======================
   POWER MODEL
   ======================= */
#define ALPHA       5.0

/* =======================
   SAFETY PARAMETERS
   ======================= */
#define ACTION_COOLDOWN 5   // seconds between mitigation actions

/* =======================
   GLOBAL STATE
   ======================= */
static int mitigation_active = 0;
static time_t last_action_time = 0;
static int original_max_freq = -1;

/* =======================
   Utility functions
   ======================= */

double read_temperature()
{
    FILE *fp = fopen(TEMP_PATH, "r");
    if (!fp) return -1.0;

    int temp_milli;
    fscanf(fp, "%d", &temp_milli);
    fclose(fp);

    return temp_milli / 1000.0;
}

double read_frequency()
{
    FILE *fp = fopen(FREQ_CUR_PATH, "r");
    if (!fp) return -1.0;

    int freq_khz;
    fscanf(fp, "%d", &freq_khz);
    fclose(fp);

    return freq_khz / 1e6;
}

int read_max_frequency()
{
    FILE *fp = fopen(FREQ_MAX_PATH, "r");
    if (!fp) return -1;

    int freq;
    fscanf(fp, "%d", &freq);
    fclose(fp);

    return freq;
}

void write_max_frequency(int freq)
{
    FILE *fp = fopen(FREQ_MAX_PATH, "w");
    if (!fp) return;

    fprintf(fp, "%d", freq);
    fclose(fp);
}

/* Placeholder CPU utilization (safe default) */
double estimate_utilization()
{
    return 0.7;
}

/* =======================
   RC Thermal Model
   ======================= */
double predict_temperature(
    double T_curr,
    double power,
    double Tamb,
    double R,
    double C,
    double dt
) {
    return T_curr + (dt / C) * (power - (T_curr - Tamb) / R);
}

/* =======================
   Safe mitigation logic
   ======================= */
int can_act()
{
    time_t now = time(NULL);
    return difftime(now, last_action_time) >= ACTION_COOLDOWN;
}

void enable_mitigation()
{
    if (mitigation_active || !can_act())
        return;

    original_max_freq = read_max_frequency();
    if (original_max_freq <= 0)
        return;

    int reduced_freq = (int)(original_max_freq * 0.7);

    write_max_frequency(reduced_freq);
    mitigation_active = 1;
    last_action_time = time(NULL);

    printf("⚠️  Mitigation ENABLED: max freq capped\n");
}

void disable_mitigation()
{
    if (!mitigation_active || !can_act())
        return;

    if (original_max_freq > 0)
        write_max_frequency(original_max_freq);

    mitigation_active = 0;
    last_action_time = time(NULL);

    printf("✅ Mitigation DISABLED: freq restored\n");
}

/* =======================
   Main control loop
   ======================= */
int main()
{
    printf("RC-Based Thermal-Aware Scheduler Controller (SAFE MODE)\n");
    printf("------------------------------------------------------\n");

    while (1) {
        double T_curr = read_temperature();
        double freq   = read_frequency();
        double util   = estimate_utilization();

        if (T_curr < 0 || freq < 0) {
            printf("Sensor read failed — entering safe mode\n");
            disable_mitigation();
            sleep((int)DT);
            continue;
        }

        double power = ALPHA * util * freq;

        double T_pred = predict_temperature(
            T_curr,
            power,
            T_AMBIENT,
            R_THERMAL,
            C_THERMAL,
            DT
        );

        printf("T=%.2f°C | T_pred=%.2f°C | f=%.2f GHz | P=%.2f W\n",
               T_curr, T_pred, freq, power);

        /* Hysteresis-based control */
        if (T_pred > T_HIGH) {
            enable_mitigation();
        }
        else if (T_pred < T_LOW) {
            disable_mitigation();
        }

        if (T_pred > T_CRITICAL) {
            printf("CRITICAL predicted temperature — strong throttling advised\n");
        }

        sleep((int)DT);
    }

    return 0;
}

