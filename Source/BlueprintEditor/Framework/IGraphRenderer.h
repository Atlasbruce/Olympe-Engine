#pragma once

// Framework-facing include for the single graph-renderer contract.
// Keeping this forwarding header preserves the Framework include layout while
// ensuring consumers receive the complete interface, not only a forward
// declaration from IGraphDocument.h.
#include "../IGraphRenderer.h"
