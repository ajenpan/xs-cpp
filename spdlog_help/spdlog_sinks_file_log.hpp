// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <spdlog/common.h>
#include <spdlog/details/circular_q.h>
#include <spdlog/details/file_helper.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/details/os.h>
#include <spdlog/details/synchronous_factory.h>
#include <spdlog/fmt/chrono.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/base_sink.h>

#include <chrono>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>

namespace spdlog {
namespace sinks {

struct daily_filename_calculator {
    // Create filename for the form basename.YYYY-MM-DD
    static filename_t calc_filename(const filename_t& filename, const tm& now_tm) {
        filename_t basename, ext;
        std::tie(basename, ext) = details::file_helper::split_by_extension(filename);
        return fmt_lib::format(SPDLOG_FMT_STRING("{}_{:02d}-{:02d}-{:02d}-{:02d}{:02d}{:02d}.log"),
                               basename, now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday, now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec);
    }

    static filename_t calc_subdir(const filename_t& filename, const tm& now_tm) {
        return fmt_lib::format("log/{:02d}-{:d}", now_tm.tm_year + 1900, now_tm.tm_mon + 1);
    }
};

template <typename Mutex, typename FileNameCalc = daily_filename_calculator>
class ag_daily_file_sink final : public base_sink<Mutex> {
  public:
    // create daily file sink which rotates on given time
    ag_daily_file_sink(filename_t base_filename, size_t max_file_size = 512 * 1024 * 1024, int rotation_hour = 0, int rotation_minute = 0, const file_event_handlers& event_handlers = {})
        : base_filename_(std::move(base_filename)), rotation_h_(rotation_hour), rotation_m_(rotation_minute), file_helper_{event_handlers}, max_file_size_(max_file_size), filenames_q_() {
        if (rotation_hour < 0 || rotation_hour > 23 || rotation_minute < 0 || rotation_minute > 59) {
            throw_spdlog_ex("ag_daily_file_sink: Invalid rotation time in ctor");
        }

        auto now = log_clock::now();

        open_next_file(now_tm(now));

        if (max_files_ > 0) {
            init_filenames_q_();
        }
    }

    filename_t filename() {
        std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
        return file_helper_.filename();
    }

  protected:
    void open_next_file(const tm& t) {
        auto subdir = FileNameCalc::calc_subdir(base_filename_, t);
        auto filename = FileNameCalc::calc_filename(base_filename_, t);
        auto filepath = subdir + "/" + filename;
        file_helper_.open(filepath, truncate_);
        rotation_tp_ = next_rotation_tp_();
        curr_file_size_ = 0;
    }

    void sink_it_(const details::log_msg& msg) override {
        auto& time = msg.time;

        bool should_rotate = (curr_file_size_ >= max_file_size_) || (time >= rotation_tp_);
        if (should_rotate) {
            open_next_file(now_tm(time));
        }
        memory_buf_t formatted;
        base_sink<Mutex>::formatter_->format(msg, formatted);
        file_helper_.write(formatted);
        curr_file_size_ += formatted.size();

        // Do the cleaning only at the end because it might throw on failure.
        if (should_rotate && max_files_ > 0) {
            delete_old_();
        }
    }

    void flush_() override {
        file_helper_.flush();
    }

  private:
    void init_filenames_q_() {
        using details::os::path_exists;

        filenames_q_ = details::circular_q<filename_t>(static_cast<size_t>(max_files_));
        std::vector<filename_t> filenames;
        auto now = log_clock::now();
        while (filenames.size() < max_files_) {
            auto filename = FileNameCalc::calc_filename(base_filename_, now_tm(now));
            if (!path_exists(filename)) {
                break;
            }
            filenames.emplace_back(filename);
            now -= std::chrono::hours(24);
        }
        for (auto iter = filenames.rbegin(); iter != filenames.rend(); ++iter) {
            filenames_q_.push_back(std::move(*iter));
        }
    }

    tm now_tm(log_clock::time_point tp) {
        time_t tnow = log_clock::to_time_t(tp);
        return spdlog::details::os::localtime(tnow);
    }

    log_clock::time_point next_rotation_tp_() {
        auto now = log_clock::now();
        tm date = now_tm(now);
        date.tm_hour = rotation_h_;
        date.tm_min = rotation_m_;
        date.tm_sec = 0;
        auto rotation_time = log_clock::from_time_t(std::mktime(&date));
        if (rotation_time > now) {
            return rotation_time;
        }
        return {rotation_time + std::chrono::hours(24)};
    }

    void delete_old_() {
        using details::os::filename_to_str;
        using details::os::remove_if_exists;

        filename_t current_file = file_helper_.filename();
        if (filenames_q_.full()) {
            auto old_filename = std::move(filenames_q_.front());
            filenames_q_.pop_front();
            bool ok = remove_if_exists(old_filename) == 0;
            if (!ok) {
                filenames_q_.push_back(std::move(current_file));
                throw_spdlog_ex("Failed removing daily file " + filename_to_str(old_filename), errno);
            }
        }
        filenames_q_.push_back(std::move(current_file));
    }

    filename_t base_filename_;
    int rotation_h_;
    int rotation_m_;
    log_clock::time_point rotation_tp_;
    details::file_helper file_helper_;
    bool truncate_ = false;
    uint16_t max_files_ = 0;
    size_t max_file_size_;
    size_t curr_file_size_ = 0;
    details::circular_q<filename_t> filenames_q_;
};

using ag_daily_file_sink_mt = ag_daily_file_sink<std::mutex>;
using ag_daily_file_sink_st = ag_daily_file_sink<details::null_mutex>;

} // namespace sinks
} // namespace spdlog