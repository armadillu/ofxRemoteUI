This is the subset of Dear ImGui used by the standalone RemoteUI ImGui client.
It comes from the upstream docking branch at commit
`5c7c293a66d7a659d4e6a1abdf18291ce004c07e`. See LICENSE.txt for its license.

The normal ofxRemoteUI addon excludes these files; the standalone client
explicitly compiles the core and GLFW/OpenGL3 backend sources. This keeps other
applications free to use their own Dear ImGui integration.
