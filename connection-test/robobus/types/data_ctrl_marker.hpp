#pragma once

namespace robobus::types {
/**
 * @enum DataCtrlMarker
 * @brief データパケットと制御パケットを区別するためのマーカー
 */
enum class DataCtrlMarker {
  /// サーバサイドデータパケットマーカー
  kServerData,
  /// サーバーサイド制御パケットマーカー
  kServerCtrl,
  /// サーバサイドデータパケットマーカー
  kClientData,
  /// サーバーサイド制御パケットマーカー
  kClientCtrl
};
}  // namespace robobus::types
