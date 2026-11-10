#pragma once
#include <string>
#include <algorithm>

// -------------------------------------------------------------
// Game clock, calendar and seasons
// -------------------------------------------------------------
// Game time advances by real time x speed (Pause / 1x / 2x / 4x).
// At 1x one game day lasts SECONDS_PER_DAY real seconds.
//
//   30 days per month, 12 months per year (360-day year)
//   Spring = Mar-May, Summer = Jun-Aug, Autumn = Sep-Nov, Winter = Dec-Feb
//
// The game starts on 1 March, Year 1 (first day of spring).

enum class Season { Spring, Summer, Autumn, Winter };

class GameClock {
public:
    static constexpr float SECONDS_PER_DAY = 4.f;
    static constexpr int DAYS_PER_MONTH = 30;
    static constexpr int MONTHS_PER_YEAR = 12;
    static constexpr int DAYS_PER_YEAR = DAYS_PER_MONTH * MONTHS_PER_YEAR;
    static constexpr int SPEED_COUNT = 4;

    // Speed steps: index 0 = paused
    static constexpr int SPEEDS[SPEED_COUNT] = {0, 1, 2, 4};

private:
    long totalDays = 0;       // whole days elapsed since 1 Mar, Year 1
    float dayProgress = 0.f;  // 0..1 through the current day
    int speedIndex = 1;       // start at 1x
    int lastSpeedIndex = 1;   // restored when unpausing

    static constexpr int START_MONTH = 3; // March

public:
    // ---- time flow ----
    // Advances by real seconds scaled by speed. Returns how many day
    // boundaries were crossed (usually 0 or 1; more at high speed / long frames).
    int advance(float realSeconds) {
        if (realSeconds <= 0.f || speedIndex == 0) return 0;
        dayProgress += realSeconds * static_cast<float>(SPEEDS[speedIndex]) / SECONDS_PER_DAY;
        int crossed = 0;
        while (dayProgress >= 1.f) {
            dayProgress -= 1.f;
            totalDays++;
            crossed++;
        }
        return crossed;
    }

    // Game-time seconds that pass for this much real time (0 when paused)
    float scaledDelta(float realSeconds) const {
        return realSeconds * static_cast<float>(SPEEDS[speedIndex]);
    }

    // ---- speed ----
    int getSpeedIndex() const { return speedIndex; }
    int getSpeedMultiplier() const { return SPEEDS[speedIndex]; }
    bool isPaused() const { return speedIndex == 0; }

    // Save games: put the calendar back exactly
    void restore(long days, float progress, int speed) {
        totalDays = days < 0 ? 0 : days;
        dayProgress = std::max(0.f, std::min(0.999f, progress));
        setSpeedIndex(speed);
    }

    void setSpeedIndex(int index) {
        if (index < 0) index = 0;
        if (index >= SPEED_COUNT) index = SPEED_COUNT - 1;
        if (index != 0) lastSpeedIndex = index;
        speedIndex = index;
    }
    void togglePause() { setSpeedIndex(isPaused() ? lastSpeedIndex : 0); }
    void faster() { setSpeedIndex(speedIndex + 1); }
    void slower() { setSpeedIndex(speedIndex - 1); }

    // ---- calendar ----
    long getTotalDays() const { return totalDays; }
    float getDayProgress() const { return dayProgress; }

    // Months counted from March of year 1
    long monthsElapsed() const { return totalDays / DAYS_PER_MONTH + (START_MONTH - 1); }
    int getDay() const { return static_cast<int>(totalDays % DAYS_PER_MONTH) + 1; }         // 1..30
    int getMonth() const { return static_cast<int>(monthsElapsed() % MONTHS_PER_YEAR) + 1; } // 1..12
    int getYear() const { return static_cast<int>(monthsElapsed() / MONTHS_PER_YEAR) + 1; }  // 1..

    static Season seasonOfMonth(int month) {
        if (month >= 3 && month <= 5) return Season::Spring;
        if (month >= 6 && month <= 8) return Season::Summer;
        if (month >= 9 && month <= 11) return Season::Autumn;
        return Season::Winter;
    }
    Season getSeason() const { return seasonOfMonth(getMonth()); }

    // 0..1 through the current season (3 months), for smooth visual transitions
    float seasonProgress() const {
        int monthInSeason = (getMonth() + 9) % 3; // Mar/Jun/Sep/Dec -> 0
        float days = static_cast<float>(monthInSeason * DAYS_PER_MONTH + getDay() - 1) + dayProgress;
        return days / static_cast<float>(3 * DAYS_PER_MONTH);
    }

    // Jumps to the first day of a month (testing / debugging)
    void setDate(int year, int month, int dayOfMonth) {
        long months = static_cast<long>(year - 1) * MONTHS_PER_YEAR + (month - 1) - (START_MONTH - 1);
        // Year 1 starts in March, so Jan/Feb of year 1 roll forward to year 2
        while (months < 0) months += MONTHS_PER_YEAR;
        totalDays = months * DAYS_PER_MONTH + (dayOfMonth - 1);
        dayProgress = 0.f;
    }

    // ---- names ----
    static std::string seasonName(Season s) {
        switch (s) {
            case Season::Spring: return "Spring";
            case Season::Summer: return "Summer";
            case Season::Autumn: return "Autumn";
            case Season::Winter: return "Winter";
        }
        return "";
    }
    static std::string monthName(int month) {
        static const char* names[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        return (month >= 1 && month <= 12) ? names[month - 1] : "?";
    }
    std::string dateString() const {
        return monthName(getMonth()) + " " + std::to_string(getDay()) + ", Year " + std::to_string(getYear());
    }
};

// The one clock driving the game (read by buildings, e.g. seasonal farm yield)
inline GameClock gameClock;

// Farm output by season: spring planting, summer growth, autumn harvest, winter fallow
inline float seasonalFarmMultiplier(Season s) {
    switch (s) {
        case Season::Spring: return 1.0f;
        case Season::Summer: return 1.3f;
        case Season::Autumn: return 1.5f;
        case Season::Winter: return 0.3f;
    }
    return 1.0f;
}
