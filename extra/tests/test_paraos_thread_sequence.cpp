
#include <iostream>
#include <memory>

#include "paraos_thread_sequence.hpp"
#include "praros_common.hpp"

using namespace paraos;

std::size_t mag_call_cnt{0};
std::size_t gyracc_call_cnt{0};

constexpr std::size_t thread_sequence_notify_give_max_numb{3};

using SequenceStaticThreadAlias = SequenceThread<OsProfiler>;

SequenceStaticThreadAlias *sequence_thread_ptr_{nullptr};

struct GyrAcc {
 public:
  GyrAcc() { std::cout << "GyrAcc Ctor" << std::endl; }

  ~GyrAcc() { std::cout << "~GyrAcc Dtor" << std::endl; }

  void Update() {
    std::cout << "GyrAcc Update()" << std::endl;
    ++gyracc_call_cnt;
  }
};

struct Mag : public Thread {
  Mag() : Thread{"Magnetic", 1024, ThreadPriority::kNormal} {
    std::cout << "Mag Ctor" << std::endl;
  }

  ~Mag() { std::cout << "~Mag Dtor" << std::endl; }

  void Run() override {
    std::cout << "Mag Run()" << std::endl;
    ++mag_call_cnt;
  }

  void Update() {
    static std::size_t cnt{0};

    if (cnt < thread_sequence_notify_give_max_numb) {
      ++cnt;
      sequence_thread_ptr_->NotifyGive();
    } else {
      // Stop thread_sequence runtime.
      sequence_thread_ptr_->Break();
    }

    std::cout << "Mag Update()" << std::endl;
    ++mag_call_cnt;
  }
};

static GyrAcc gyr_acc_local{};
static Mag mag_local{};

const TaskSequence task_sequence[] = {
    SCHED_TASK_CLASS(GyrAcc, &gyr_acc_local, Update, 50.0f, 200, 9),
    SCHED_TASK_CLASS(Mag, &mag_local, Update, 20.0f, 200, 9),
    SCHED_TASK_CLASS(Mag, &mag_local, Run, 20.0f, 200, 9)};

TaskSequenceInfo task_sequence_info[PARAOS_ARRAY_SIZE(task_sequence)];

int main() {
  SequenceStaticThreadAlias sequence_static(
      "Sequence static", 1024, ThreadPriority::kNormal, task_sequence,
      task_sequence_info);
  sequence_thread_ptr_ = &sequence_static;
  sequence_static.NotifyGive();

  Thread::StartScheduler();
  Thread::DeleteAll();

  // number of calling GyrAcc must be equal numbers of given binary semaphore
  // from  thread_sequence.NotifyGive();
  Ensures(gyracc_call_cnt == (thread_sequence_notify_give_max_numb + 1u));
  Ensures(gyracc_call_cnt == mag_call_cnt / 2);

  return 0;
}
