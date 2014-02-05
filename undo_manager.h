// Copyright...

#ifndef PDFSKETCH_UNDO_MANAGER_H__
#define PDFSKETCH_UNDO_MANAGER_H__

#include <deque>
#include <functional>

namespace pdfsketch{

class UndoManagerDelegage {
 public:
  void SetUndoEnabled(bool enabled) = 0;
  void SetRedoEnabled(bool enabled) = 0;
};

class UndoManager {
 public:
  UndoManager();

  void SetDelegate(UndoManagerDelegage* delegate) {
    delegate_ = delegate;
  }
  void AddClosure(std::function<void ()> func);
  void PerformUndo();
  void PerformRedo();
  
 private:
  void UpdateDelegate();
  void PerformUndoImpl(std::deque<function<void ()>>* undo,
                       std::deque<function<void ()>>* redo);

  UndoManagerDelegate* delegate_;
  bool undo_in_progress_;
  std::deque<function<void ()>> undo_ops_;
  std::deque<function<void ()>> redo_ops_;
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_UNDO_MANAGER_H__
