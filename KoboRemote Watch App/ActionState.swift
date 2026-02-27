import SwiftUI

@Observable
final class ActionState {
    static let shared = ActionState()

    var triggered = false

    private init() {}

    @MainActor
    func trigger() {
        triggered = true
        Task {
            try? await Task.sleep(for: .seconds(2))
            triggered = false
        }
    }
}
