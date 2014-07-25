// Copyright...

#ifndef PDFSKETCH_UNDO_MANAGER_H__
#define PDFSKETCH_UNDO_MANAGER_H__

#include <deque>
#include <functional>
#include <vector>

namespace pdfsketch{

class UndoManagerDelegate {
 public:
  virtual void SetUndoEnabled(bool enabled) = 0;
  virtual void SetRedoEnabled(bool enabled) = 0;
};

class ScopedUndoAggregator;

class UndoManager {
 public:
  void SetDelegate(UndoManagerDelegate* delegate) {
    delegate_ = delegate;
  }
  void AddClosure(std::function<void ()> func);
  void PerformUndo();
  void PerformRedo();
  
  ScopedUndoAggregator* aggregator() const {
    return aggregator_;
  }
  void set_aggregator(ScopedUndoAggregator* aggregator) {
    aggregator_ = aggregator;
  }

 private:
  void UpdateDelegate();
  void PerformUndoImpl(std::deque<std::function<void ()>>* ops);

  UndoManagerDelegate* delegate_{nullptr};
  bool undo_in_progress_{false};
  bool redo_in_progress_{false};
  std::deque<std::function<void ()>> undo_ops_;
  std::deque<std::function<void ()>> redo_ops_;

  ScopedUndoAggregator* aggregator_{nullptr};
};

class ScopedUndoAggregator {
 public:
  ScopedUndoAggregator(UndoManager* undo_manager);
  ~ScopedUndoAggregator();
  void AddClosure(std::function<void ()> func) {
    ops_.push_back(func);
  }
 private:
  std::vector<std::function<void ()>> ops_;
  UndoManager* undo_manager_{nullptr};
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_UNDO_MANAGER_H__
