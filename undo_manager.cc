// Copyright...

#include "undo_manager.h"

#include <stdlib.h>

using std::function;

namespace pdfsketch{

UndoManager::UndoManager()
    : delegate_(NULL),
      undo_in_progress_(false),
      redo_in_progress_(false) {}

void UndoManager::AddClosure(std::function<void ()> func) {
  if (!undo_in_progress_) {
    if (!redo_in_progress_)
      redo_ops_.clear();
    undo_ops_.push_back(func);
    if (undo_ops_.size() > 3)
      undo_ops_.pop_front();
  } else {
    redo_ops_.push_back(func);
  }
  UpdateDelegate();
}

void UndoManager::PerformUndo() {
  if (undo_ops_.empty())
    return;
  undo_in_progress_ = true;
  PerformUndoImpl(&undo_ops_);
  undo_in_progress_ = false;
  UpdateDelegate();
}

void UndoManager::PerformRedo() {
  if (redo_ops_.empty())
    return;
  redo_in_progress_ = true;
  PerformUndoImpl(&redo_ops_);
  redo_in_progress_ = false;
  UpdateDelegate();
}

void UndoManager::UpdateDelegate() {
  if (!delegate_)
    return;
  delegate_->SetUndoEnabled(!undo_ops_.empty());
  delegate_->SetRedoEnabled(!redo_ops_.empty());
}

void UndoManager::PerformUndoImpl(std::deque<function<void ()>>* ops) {
  ops->back()();
  ops->pop_back();
}
  
}  // namespace pdfsketch
