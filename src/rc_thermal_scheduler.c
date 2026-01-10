/*
 * RC-based Thermal-Aware Scheduler (User Space)
 *
 * Course Project Starter Code
 *
 * What this does:
 *  - Reads CPU temperature from Linux thermal sysfs
 *  - Estimates power using CPU utilization + frequency
 *  - Predicts future temperature using RC thermal model
 *  - Takes scheduling-related actions (hooks provided)
 *
 * What this does NOT do (yet):
 *  - No kernel modification
 *  - No aggressive task killing
 *
 * Compile:
 *   gcc rc_thermal_scheduler.c -o rc_sched -lm
 *
 * Run (needs root for freq control):
 *   sudo ./rc_sched
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#define TEMP_PATH "/sys/class/thermal/thermal_zone0/temp"
#define FREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"

/* =======================
   RC MODEL PARAMETERS
   ======================= */
#define R_THERMAL   1.0     // Thermal resistance (tunable)
#define C_THERMAL   10.0    // Thermal capacitance (tunable)
#define T_AMBIENT   30.0    // Ambient temperature (°C)
#define DT          1.0     // Control interval (seconds)

/* =======================
   THERMAL LIMITS
   ======================= */
#define T_SAFE      75.0
#define T_CRITICAL  85.0

/* =======================
   POWER MODEL PARAMS
   ======================= */
#define ALPHA       5.0     // Power scaling constant

/* =======================
   Utility functions
   ======================= */

/* Read temperature in Celsius */
double read_temperature()
{
    FILE *fp = fopen(TEMP_PATH, "r");
    if (!fp) {
        perror("Temperature read failed");
        return -1.0;
    }

    int temp_milli;
    fscanf(fp, "%d", &temp_milli);
    fclose(fp);

    return temp_milli / 1000.0;
}

/* Read CPU frequency in GHz */
double read_frequency()
{
    FILE *fp = fopen(FREQ_PATH, "r");
    if (!fp) {
        perror("Frequency read failed");
        return 1.0;
    }

    int freq_khz;
    fscanf(fp, "%d", &freq_khz);
    fclose(fp);

    return freq_khz / 1e6;
}

/* Dummy CPU utilization estimator (replace later) */
double estimate_utilization()
{
    /* Simple placeholder:
       Assume high utilization for demo */
    return 0.8;
}

/* =======================
   RC thermal prediction
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
   Scheduling actions
   ======================= */
void apply_mitigation(double T_pred)
{
    if (T_pred > T_CRITICAL) {
        printf("!!! CRITICAL !!!: Predicted temp %.2f°C — migrate tasks / throttle hard\n", T_pred);
        /* TODO:
         * - sched_setaffinity()
         * - aggressive freq scaling
         */
    }
    else if (T_pred > T_SAFE) {
        printf("!!!  WARNING !!!: Predicted temp %.2f°C — reducing frequency\n", T_pred);
        /* TODO:
         * - reduce CPU frequency
         */
    }
    else {
        printf("✅ OK: Predicted temp %.2f°C\n", T_pred);
    }
}

/* =======================
   Main control loop
   ======================= */
int main()
{
    printf("RC-Based Thermal-Aware Scheduler Started\n");
    printf("----------------------------------------\n");

    while (1) {
        double T_curr = read_temperature();
        double freq   = read_frequency();
        double util   = estimate_utilization();

        if (T_curr < 0)
            continue;

        /* Simple power model */
        double power = ALPHA * util * freq;

        double T_pred = predict_temperature(
            T_curr,
            power,
            T_AMBIENT,
            R_THERMAL,
            C_THERMAL,
            DT
        );

        printf("T_curr = %.2f°C | T_pred = %.2f°C | P = %.2f W | f = %.2f GHz\n",
               T_curr, T_pred, power, freq);

        apply_mitigation(T_pred);

        sleep((int)DT);
    }

    return 0;
}

