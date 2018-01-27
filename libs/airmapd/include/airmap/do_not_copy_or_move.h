#ifndef AIRMAP_DO_NOT_COPY_OR_MOVE_H_
#define AIRMAP_DO_NOT_COPY_OR_MOVE_H_

namespace airmap {

/// @cond
struct DoNotCopyOrMove {
  DoNotCopyOrMove()                       = default;
  DoNotCopyOrMove(const DoNotCopyOrMove&) = delete;
  DoNotCopyOrMove(DoNotCopyOrMove&&)      = delete;
  virtual ~DoNotCopyOrMove()              = default;
  DoNotCopyOrMove& operator=(const DoNotCopyOrMove&) = delete;
  DoNotCopyOrMove& operator=(DoNotCopyOrMove&&) = delete;
};
/// @endcond

}  // namespace airmap

#endif  // AIRMAP_DO_NOT_COPY_OR_MOVE_H_
