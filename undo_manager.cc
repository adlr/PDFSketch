// Copyright...

#include "undo_manager.h"

namespace pdfsketch{

UndoManager::UndoManager()
    : delegate_(NULL),
      undo_in_progress_(false) {}

void UndoManager::AddClosure(std::function<void ()> func) {
  if (!undo_in_progress_) {
    undo_ops_.push_back(func);
  } else {
    redo_ops_.push_back(func);
  }
}

void UndoManager::PerformUndo() {
  PerformUndoImpl(&undo_ops, &redo_ops_);
  UpdateDelegate();
}
void UndoManager::PerformRedo() {
  PerformUndoImpl(&redo_ops, &undo_ops_);
  UpdateDelegate();
}

void UndoManager::UpdateDelegate() {
  if (!delegate_)
    return;
  delegate_->SetUndoEnabled(!undo_ops_.empty());
  delegate_->SetRedoEnabled(!redo_ops_.empty());
}

void UndoManager::PerformUndoImpl(std::deque<function<void ()>>* undo,
                                  std::deque<function<void ()>>* redo) {
  
}
  
}  // namespace pdfsketch

#endif  // PDFSKETCH_UNDO_MANAGER_H__
