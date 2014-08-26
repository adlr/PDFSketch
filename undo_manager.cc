// Copyright...

#include "undo_manager.h"

#include <stdio.h>
#include <stdlib.h>

using std::function;
using std::unique_ptr;
using std::vector;

namespace pdfsketch {

void UndoManager::AddUndoOp(unique_ptr<UndoOp> op) {
  if (aggregator_) {
    aggregator_->AddUndoOp(std::move(op));
    return;
  }
  if (!undo_in_progress_) {
    if (!redo_in_progress_)
      redo_ops_.clear();
    if (undo_ops_.empty() || !undo_ops_.back()->Merge(*op))
      undo_ops_.push_back(std::move(op));
    while (undo_ops_.size() > 10 &&
           !(undo_ops_.front()->Type() == UndoOp::UndoType::Marker))
      undo_ops_.pop_front();
  } else {
    redo_ops_.push_back(std::move(op));
  }
  UpdateDelegate();
}

void UndoManager::AddClosure(std::function<void ()> func) {
  AddUndoOp(unique_ptr<UndoOp>(new FunctorUndoOp(func)));
}

void UndoManager::PerformUndo() {
  if (undo_ops_.empty() || undo_ops_.back()->Type() == UndoOp::UndoType::Marker)
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
  delegate_->SetUndoEnabled(
      !undo_ops_.empty() &&
      undo_ops_.back()->Type() != UndoOp::UndoType::Marker);
  delegate_->SetRedoEnabled(!redo_ops_.empty());
}

void UndoManager::PerformUndoImpl(std::deque<unique_ptr<UndoOp>>* ops) {
  while (ops->back()->Type() == UndoOp::UndoType::Marker)
    ops->pop_back();
  if (ops->empty())
    return;
  ops->back()->Exec(this);
  ops->pop_back();
}
  
void UndoManager::SetMarker() {
  AddUndoOp(unique_ptr<UndoOp>(new MarkerUndoOp()));
}

void UndoManager::ClearFromMarker() {
  while (!undo_ops_.empty()) {
    bool last = undo_ops_.back()->Type() == UndoOp::UndoType::Marker;
    undo_ops_.pop_back();
    if (last)
      return;
  }
}

bool UndoManager::OpsAddedAfterMarker() const {
  return !undo_ops_.empty() &&
      !undo_ops_.back()->Type() == UndoOp::UndoType::Marker;
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
    undo_manager_->AddUndoOp(std::move(ops_[0]));
  } else if (ops_.size() > 1) {
    // Make a group closure
    // We have to pull the pointers from out of the unique_ptr's here
    // b/c we need to move them to the lambda. You can only capture
    // something copyable, and unique_ptr's are not copyable.
    vector<UndoOp*> ops_copy;
    for (unique_ptr<UndoOp>& ptr : ops_) {
      ops_copy.push_back(ptr.release());
    }
    UndoManager* undo_manager = undo_manager_;
    undo_manager_->AddClosure([undo_manager, ops_copy] () {
        ScopedUndoAggregator undo_aggregator(undo_manager);
        // Replay the items in reverse order
        for (auto it = ops_copy.rbegin(), e = ops_copy.rend();
             it != e; ++it) {
          (*it)->Exec(undo_manager);
          delete *it;
        }
      });
  }
}

}  // namespace pdfsketch
