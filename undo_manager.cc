// Copyright...

#include "undo_manager.h"

#include <stdio.h>
#include <stdlib.h>

using std::function;
using std::vector;

namespace pdfsketch{

void UndoManager::AddClosure(std::function<void ()> func) {
  if (aggregator_) {
    aggregator_->AddClosure(func);
    return;
  }
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
  
ScopedUndoAggregator::ScopedUndoAggregator(
    UndoManager* undo_manager)
    : undo_manager_(undo_manager) {
  if (undo_manager_->aggregator()) {
    // Already have aggregator. Do nothing.
    return;
  }
  undo_manager_->set_aggregator(this);
}

ScopedUndoAggregator::~ScopedUndoAggregator() {
  if (undo_manager_->aggregator() != this) {
    // We aren't active.
    return;
  }
  undo_manager_->set_aggregator(nullptr);
  if (ops_.size() == 1) {
    // Just one op. No need for group.
    undo_manager_->AddClosure(ops_[0]);
  } else if (ops_.size() > 1) {
    // Make a group closure
    vector<std::function<void ()>> ops_copy;
    swap(ops_copy, ops_);
    UndoManager* undo_manager = undo_manager_;
    undo_manager_->AddClosure([undo_manager, ops_copy] () {
        ScopedUndoAggregator undo_aggregator(undo_manager);
        // Replay the items in reverse order
        for (auto it = ops_copy.rbegin(), e = ops_copy.rend();
             it != e; ++it) {
          (*it)();
        }
      });
  }
}

}  // namespace pdfsketch
