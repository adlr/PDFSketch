// Copyright...

#ifndef PDFSKETCH_UNDO_MANAGER_H__
#define PDFSKETCH_UNDO_MANAGER_H__

#include <deque>
#include <functional>
#include <string>
#include <vector>

namespace pdfsketch{

class UndoManager;

class UndoManagerDelegate {
 public:
  virtual void SetUndoEnabled(bool enabled) = 0;
  virtual void SetRedoEnabled(bool enabled) = 0;
};

class ScopedUndoAggregator;

class UndoOp {
 public:
  enum UndoType {
    Functor,
    Marker,
    TextAreaTransform
  };

  virtual ~UndoOp() {}
  virtual void Exec(UndoManager* manager) {}
  virtual UndoType Type() const = 0;
  // Returns true if 'that' is successfully merged into this.
  virtual bool Merge(const UndoOp& that) { return false; }
};

class FunctorUndoOp : public UndoOp {
 public:
  virtual ~FunctorUndoOp() {}
  explicit FunctorUndoOp(const std::function<void ()>& fn) : fn_(fn) {}
  void Exec(UndoManager* manager) override { fn_(); }
  virtual UndoType Type() const { return Functor; }
 private:
  std::function<void ()> fn_;
};

class MarkerUndoOp : public UndoOp {
 public:
  virtual ~MarkerUndoOp() {}
  virtual UndoType Type() const { return Marker; }
};

class UndoManager {
 public:
  void SetDelegate(UndoManagerDelegate* delegate) {
    delegate_ = delegate;
  }
  void AddUndoOp(std::unique_ptr<UndoOp> op);
  void AddClosure(std::function<void ()> func);
  // Add an operation that, when performed, will modify str by calling
  // str->replace(start, end, value) or equivalent.
  // The advantage of this is that multiple calls can be coalesced.
  void PerformUndo();
  void PerformRedo();
  
  // After a marker is set, Closures can be added as usual. However,
  // a call to ClearFromMarker() will erase the marker and all ops that
  // were added after it.
  void SetMarker();
  void ClearFromMarker();
  bool OpsAddedAfterMarker() const;

  ScopedUndoAggregator* aggregator() const {
    return aggregator_;
  }
  void set_aggregator(ScopedUndoAggregator* aggregator) {
    aggregator_ = aggregator;
  }

 private:
  void UpdateDelegate();
  void PerformUndoImpl(std::deque<std::unique_ptr<UndoOp>>* ops);

  UndoManagerDelegate* delegate_{nullptr};
  bool undo_in_progress_{false};
  bool redo_in_progress_{false};
  // Must use pointers in the deque so UndoOp subclass members aren't
  // sliced off.
  std::deque<std::unique_ptr<UndoOp>> undo_ops_;
  std::deque<std::unique_ptr<UndoOp>> redo_ops_;

  ScopedUndoAggregator* aggregator_{nullptr};
};

class ScopedUndoAggregator {
 public:
  ScopedUndoAggregator(UndoManager* undo_manager);
  ~ScopedUndoAggregator();
  void AddUndoOp(std::unique_ptr<UndoOp> op) {
    ops_.push_back(std::move(op));
  }
 private:
  std::vector<std::unique_ptr<UndoOp>> ops_;
  UndoManager* undo_manager_{nullptr};
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_UNDO_MANAGER_H__
