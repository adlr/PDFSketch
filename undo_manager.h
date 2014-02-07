// Copyright...

#ifndef PDFSKETCH_UNDO_MANAGER_H__
#define PDFSKETCH_UNDO_MANAGER_H__

#include <deque>
#include <functional>

namespace pdfsketch{

class UndoManagerDelegate {
 public:
  virtual void SetUndoEnabled(bool enabled) = 0;
  virtual void SetRedoEnabled(bool enabled) = 0;
};

class UndoManager {
 public:
  UndoManager();

  void SetDelegate(UndoManagerDelegate* delegate) {
    delegate_ = delegate;
  }
  void AddClosure(std::function<void ()> func);
  void PerformUndo();
  void PerformRedo();
  
 private:
  void UpdateDelegate();
  void PerformUndoImpl(std::deque<std::function<void ()>>* ops);

  UndoManagerDelegate* delegate_;
  bool undo_in_progress_;
  bool redo_in_progress_;
  std::deque<std::function<void ()>> undo_ops_;
  std::deque<std::function<void ()>> redo_ops_;
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_UNDO_MANAGER_H__
