// ref: https://github.com/Dreamtowards/Ethertia/blob/main/src/ethertia/util/Profiler.h
#pragma once

#include <cassert>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#define PROFILE_VN_CONCAT_INNR(a, b) a##b
#define PROFILE_VN_CONCAT(a, b) PROFILE_VN_CONCAT_INNR(a, b)
#define PROFILE(x) auto PROFILE_VN_CONCAT(_profiler, __COUNTER__) = g_FrameProfiler.pushCaller(x)
#define PROFILE_PREC(x) auto PROFILE_VN_CONCAT(_profiler, __COUNTER__) = g_PreComputeProfiler.pushCaller(x)
#define PROFILE_STEP(x) auto PROFILE_VN_CONCAT(_profiler, __COUNTER__) = g_StepProfiler.pushCaller(x)
#define PROFILE_X(p, x) auto PROFILE_VN_CONCAT(_profiler, __COUNTER__) = p.pushCaller(x)

namespace Util
{

    struct DestructorFuncCaller
    {
        std::function<void()> func;

        ~DestructorFuncCaller() { func(); }
    };

    struct Profiler
    {
        struct Section
        {
            std::string          name;
            std::vector<Section> sections;
            Section*             parent = nullptr;

            size_t num_exec   = 0;
            double sum_time   = 0.0;
            double begin_time = 0.0;
            double avg_time   = 0.0;
            double last_time  = 0.0;

            Section& find(std::string_view _n)
            {
                assert(_n.length() > 0);
                for (Section& s : sections)
                {
                    if (s.name == _n)
                        return s;
                }
                Section& sec = sections.emplace_back();
                sec.name     = _n;
                return sec;
            }
            void reset()
            {
                sum_time   = 0;
                num_exec   = 0;
                begin_time = 0;
                avg_time   = 0;
                last_time  = 0;
                for (Section& s : sections)
                {
                    s.reset();
                }
            }
        };
        std::thread::id m_local_thread_id;

        Section  m_root_section;
        Section* m_current_section   = &m_root_section;
        Section* section_to_be_clear = nullptr; // clear section should after it's pop().

    public:
        static double nanoseconds()
        {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(
                       std::chrono::high_resolution_clock::now().time_since_epoch())
                .count();
        }

        void push(std::string_view name)
        {
            if (m_root_section.sections.empty())
            {
                m_local_thread_id = std::this_thread::get_id();
            }
            else
            {
                assert(m_RootSection.sections.size() == 1);
                assert(m_LocalThreadId == std::this_thread::get_id());
            }

            Section& sec = m_current_section->find(name);

            sec.parent     = m_current_section; // if (sec.parent == nullptr)
            sec.begin_time = nanoseconds();

            m_current_section = &sec;
        }

        void pop()
        {
            if (section_to_be_clear == m_current_section)
            {
                section_to_be_clear = nullptr;
                auto* parent        = m_current_section->parent;
                m_current_section->reset();
                m_current_section = parent;
                return;
            }

            Section& sec = *m_current_section;

            double dur = (nanoseconds() - sec.begin_time) / 1e9;
            sec.num_exec++;
            sec.sum_time += dur;
            sec.last_time = dur;
            sec.avg_time  = sec.sum_time / sec.num_exec;

            m_current_section = m_current_section->parent;
        }

        Section& getRootSection()
        {
            if (m_root_section.sections.empty())
                return m_root_section; // temporary solution when no section recorded.
            assert(m_RootSection.sections.size() == 1);
            return m_root_section.sections.at(0);
        }

        // when we want reset/clear profiler data, we cannot just direct clear,
        // it consist push/pop,. clear should be after last pop.
        void laterClearRootSection()
        {
            if (m_current_section == &m_root_section)
            { // just clear directly.
                m_root_section.reset();
            }
            else
            {
                // Delay clear after last pop.
                section_to_be_clear = &getRootSection();
            }
        }

        // clang-format off
    [[nodiscard]] 
    DestructorFuncCaller pushCaller(std::string_view name)
    {
        push(name);
        return DestructorFuncCaller{ [this]() {pop();} };
    }
        // clang-format on
    };

} // namespace Util