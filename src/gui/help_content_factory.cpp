/**
 * @file help_content_factory.cpp
 * @brief Factory for creating comprehensive tutorial content for ECScope
 */

#include "ecscope/gui/help_system.hpp"
#include <memory>
#include <sstream>

namespace ecscope::gui::help {

// =============================================================================
// HELP CONTENT FACTORY IMPLEMENTATION
// =============================================================================

std::unique_ptr<HelpArticle> HelpContentFactory::CreateArticle(
    const std::string& id,
    const std::string& title,
    const std::string& markdownContent) {
    
    auto article = std::make_unique<HelpArticle>(id, title);
    
    // Parse markdown content into sections
    // This is a simplified parser - in production, use a proper markdown library
    HelpArticle::Section section;
    section.title = "Content";
    section.content = markdownContent;
    article->AddSection(section);
    
    return article;
}

std::unique_ptr<Tutorial> HelpContentFactory::CreateBasicTutorial(
    const std::string& id,
    const std::string& name,
    HelpCategory category) {
    
    auto tutorial = std::make_unique<Tutorial>(id, name);
    tutorial->SetCategory(category);
    return tutorial;
}

std::unique_ptr<GuidedTour> HelpContentFactory::CreateGuidedTour(
    const std::string& id,
    const std::string& title,
    const std::vector<GuidedTour::Waypoint>& waypoints) {
    
    auto tour = std::make_unique<GuidedTour>(id, title);
    for (const auto& waypoint : waypoints) {
        tour->AddWaypoint(waypoint);
    }
    return tour;
}

// =============================================================================
// GETTING STARTED TUTORIAL
// =============================================================================

std::unique_ptr<Tutorial> HelpContentFactory::CreateGettingStartedTutorial() {
    auto tutorial = std::make_unique<Tutorial>("getting_started", "Getting Started with ECScope");
    tutorial->SetCategory(HelpCategory::GettingStarted);
    tutorial->SetEstimatedTime(15);
    tutorial->m_description = "Learn the basics of ECScope game engine and create your first project";
    tutorial->m_targetLevel = UserLevel::Beginner;
    
    // Step 1: Welcome
    auto step1 = std::make_unique<TutorialStep>(
        "Welcome to ECScope! This tutorial will guide you through the basics of using the engine. "
        "Let's start by exploring the main interface."
    );
    step1->SetSkippable(true);
    tutorial->AddStep(std::move(step1));
    
    // Step 2: Interface Overview
    auto step2 = std::make_unique<TutorialStep>(
        "The main window contains several key areas:\n"
        "- Menu Bar: Access all major functions\n"
        "- Dashboard: Quick access to common tasks\n"
        "- Viewport: View and interact with your game world\n"
        "- Inspector: Edit properties of selected objects"
    );
    step2->SetHighlight(ImVec2(0, 0), ImVec2(1920, 30)); // Highlight menu bar
    step2->SetHint("Take a moment to familiarize yourself with each area");
    tutorial->AddStep(std::move(step2));
    
    // Step 3: Create New Project
    auto step3 = std::make_unique<TutorialStep>(
        "Let's create a new project. Click on File > New Project in the menu bar."
    );
    step3->SetArrow(ImVec2(960, 540), ImVec2(100, 30));
    step3->SetAction(TutorialAction::Click, []() {
        // Simulate opening new project dialog
    });
    step3->SetValidation([]() {
        // Check if new project dialog is open
        return false; // Placeholder
    });
    tutorial->AddStep(std::move(step3));
    
    // Step 4: Configure Project
    auto step4 = std::make_unique<TutorialStep>(
        "Enter a name for your project and choose a template. "
        "For this tutorial, select the 'Empty' template."
    );
    step4->SetHint("Project names should be descriptive and avoid special characters");
    tutorial->AddStep(std::move(step4));
    
    // Step 5: Navigate Viewport
    auto step5 = std::make_unique<TutorialStep>(
        "Great! Your project is created. Now let's learn viewport navigation:\n"
        "- Left-click + drag: Rotate camera\n"
        "- Middle-click + drag: Pan camera\n"
        "- Scroll wheel: Zoom in/out\n"
        "Try moving around the viewport now."
    );
    step5->SetHint("Hold Shift while moving for faster navigation");
    tutorial->AddStep(std::move(step5));
    
    // Step 6: Create First Entity
    auto step6 = std::make_unique<TutorialStep>(
        "Let's create your first entity. Right-click in the viewport and select "
        "'Create Entity > 3D Object > Cube' from the context menu."
    );
    step6->SetAction(TutorialAction::Click, []() {
        // Show context menu
    });
    tutorial->AddStep(std::move(step6));
    
    // Step 7: Select and Transform
    auto step7 = std::make_unique<TutorialStep>(
        "Click on the cube to select it. You can now use the transform tools:\n"
        "- W: Move tool\n"
        "- E: Rotate tool\n"
        "- R: Scale tool\n"
        "Try moving the cube around."
    );
    step7->SetHint("Hold Ctrl while transforming to snap to grid");
    tutorial->AddStep(std::move(step7));
    
    // Step 8: Add Component
    auto step8 = std::make_unique<TutorialStep>(
        "With the cube selected, look at the Inspector panel. "
        "Click 'Add Component' and add a 'Rigidbody' component to make the cube affected by physics."
    );
    step8->SetHighlight(ImVec2(1520, 100), ImVec2(400, 600));
    tutorial->AddStep(std::move(step8));
    
    // Step 9: Play Mode
    auto step9 = std::make_unique<TutorialStep>(
        "Let's see physics in action! Click the Play button in the toolbar or press Space "
        "to enter play mode. The cube should fall due to gravity."
    );
    step9->SetArrow(ImVec2(960, 540), ImVec2(960, 50));
    tutorial->AddStep(std::move(step9));
    
    // Step 10: Conclusion
    auto step10 = std::make_unique<TutorialStep>(
        "Congratulations! You've learned the basics of ECScope:\n"
        "- Creating projects\n"
        "- Navigating the viewport\n"
        "- Creating and transforming entities\n"
        "- Adding components\n"
        "- Using play mode\n\n"
        "Continue with more tutorials to master ECScope!"
    );
    step10->SetSkippable(true);
    tutorial->AddStep(std::move(step10));
    
    return tutorial;
}

// =============================================================================
// ECS TUTORIAL
// =============================================================================

std::unique_ptr<Tutorial> HelpContentFactory::CreateECSTutorial() {
    auto tutorial = std::make_unique<Tutorial>("ecs_tutorial", "Understanding Entity-Component-System");
    tutorial->SetCategory(HelpCategory::ECS);
    tutorial->SetEstimatedTime(20);
    tutorial->m_description = "Deep dive into ECScope's Entity-Component-System architecture";
    tutorial->m_targetLevel = UserLevel::Intermediate;
    tutorial->SetPrerequisites({"getting_started"});
    
    // Step 1: ECS Introduction
    auto step1 = std::make_unique<TutorialStep>(
        "The Entity-Component-System (ECS) is the core architecture of ECScope.\n\n"
        "- Entities: Unique identifiers for game objects\n"
        "- Components: Data containers that define properties\n"
        "- Systems: Logic that processes entities with specific components"
    );
    tutorial->AddStep(std::move(step1));
    
    // Step 2: Create Entity
    auto step2 = std::make_unique<TutorialStep>(
        "Let's create an entity programmatically. Open the ECS Inspector "
        "and click 'Create Entity' button."
    );
    step2->SetHighlight(ImVec2(200, 100), ImVec2(300, 600));
    tutorial->AddStep(std::move(step2));
    
    // Step 3: Understanding Components
    auto step3 = std::make_unique<TutorialStep>(
        "Components are pure data structures. Let's add multiple components:\n"
        "1. Transform: Position, rotation, scale\n"
        "2. MeshRenderer: Visual representation\n"
        "3. Collider: Physics collision shape\n\n"
        "Notice how each component adds specific functionality."
    );
    tutorial->AddStep(std::move(step3));
    
    // Step 4: Component Composition
    auto step4 = std::make_unique<TutorialStep>(
        "The power of ECS comes from component composition. "
        "Try different combinations:\n"
        "- Transform + MeshRenderer = Static visual object\n"
        "- Transform + Collider + Rigidbody = Physics object\n"
        "- Transform + Light = Light source"
    );
    step4->SetHint("Components can be enabled/disabled without removing them");
    tutorial->AddStep(std::move(step4));
    
    // Step 5: Create System
    auto step5 = std::make_unique<TutorialStep>(
        "Now let's look at Systems. Open the Systems panel and observe "
        "the active systems processing your entities."
    );
    step5->SetHighlight(ImVec2(500, 100), ImVec2(400, 600));
    tutorial->AddStep(std::move(step5));
    
    // Step 6: Query Builder
    auto step6 = std::make_unique<TutorialStep>(
        "Systems use queries to find entities. Open the Query Builder and create a query:\n"
        "- Include: Transform, MeshRenderer\n"
        "- Exclude: Disabled\n\n"
        "This finds all visible entities."
    );
    tutorial->AddStep(std::move(step6));
    
    // Step 7: Archetypes
    auto step7 = std::make_unique<TutorialStep>(
        "ECScope groups entities with the same component combination into Archetypes. "
        "Open the Archetype Viewer to see memory layout and performance metrics."
    );
    step7->SetHint("Archetypes improve cache locality and iteration speed");
    tutorial->AddStep(std::move(step7));
    
    // Step 8: Performance Benefits
    auto step8 = std::make_unique<TutorialStep>(
        "ECS provides excellent performance through:\n"
        "- Data-oriented design\n"
        "- Cache-friendly memory layout\n"
        "- Parallel processing capabilities\n"
        "- Efficient queries and iterations"
    );
    tutorial->AddStep(std::move(step8));
    
    // Step 9: Best Practices
    auto step9 = std::make_unique<TutorialStep>(
        "ECS Best Practices:\n"
        "- Keep components small and focused\n"
        "- Avoid component dependencies\n"
        "- Use tags for marker components\n"
        "- Profile archetype changes\n"
        "- Batch similar operations"
    );
    tutorial->AddStep(std::move(step9));
    
    // Step 10: Advanced Features
    auto step10 = std::make_unique<TutorialStep>(
        "Explore advanced ECS features:\n"
        "- Chunk iteration for bulk operations\n"
        "- Component pools for frequent add/remove\n"
        "- Entity relationships and hierarchies\n"
        "- Reactive systems for event handling\n\n"
        "Check the advanced tutorials for more!"
    );
    tutorial->AddStep(std::move(step10));
    
    return tutorial;
}

// =============================================================================
// RENDERING TUTORIAL
// =============================================================================

std::unique_ptr<Tutorial> HelpContentFactory::CreateRenderingTutorial() {
    auto tutorial = std::make_unique<Tutorial>("rendering_tutorial", "Mastering Rendering in ECScope");
    tutorial->SetCategory(HelpCategory::Rendering);
    tutorial->SetEstimatedTime(25);
    tutorial->m_description = "Learn about ECScope's powerful rendering pipeline and visual effects";
    tutorial->m_targetLevel = UserLevel::Intermediate;
    
    // Step 1: Rendering Pipeline Overview
    auto step1 = std::make_unique<TutorialStep>(
        "ECScope uses a modern rendering pipeline:\n"
        "1. Scene traversal and culling\n"
        "2. Render queue sorting\n"
        "3. Shadow mapping\n"
        "4. Forward/Deferred rendering\n"
        "5. Post-processing effects"
    );
    tutorial->AddStep(std::move(step1));
    
    // Step 2: Materials and Shaders
    auto step2 = std::make_unique<TutorialStep>(
        "Open the Material Editor. Materials define how surfaces appear:\n"
        "- Albedo: Base color\n"
        "- Metallic: Metal-like properties\n"
        "- Roughness: Surface smoothness\n"
        "- Normal: Surface detail"
    );
    step2->SetHighlight(ImVec2(900, 100), ImVec2(600, 800));
    tutorial->AddStep(std::move(step2));
    
    // Step 3: Create Material
    auto step3 = std::make_unique<TutorialStep>(
        "Let's create a new material:\n"
        "1. Click 'Create New Material'\n"
        "2. Choose 'PBR Standard' shader\n"
        "3. Assign textures by dragging from asset browser\n"
        "4. Adjust parameters with sliders"
    );
    step3->SetHint("Use the preview sphere to see changes in real-time");
    tutorial->AddStep(std::move(step3));
    
    // Step 4: Lighting Setup
    auto step4 = std::make_unique<TutorialStep>(
        "Good lighting is crucial. Add different light types:\n"
        "- Directional: Sun/moon light\n"
        "- Point: Omnidirectional light\n"
        "- Spot: Focused cone of light\n"
        "- Area: Soft rectangular light"
    );
    tutorial->AddStep(std::move(step4));
    
    // Step 5: Shadows
    auto step5 = std::make_unique<TutorialStep>(
        "Configure shadow settings:\n"
        "1. Enable shadows on lights\n"
        "2. Adjust shadow resolution\n"
        "3. Set shadow distance\n"
        "4. Configure cascade settings for directional lights"
    );
    step5->SetHint("Higher shadow resolution improves quality but impacts performance");
    tutorial->AddStep(std::move(step5));
    
    // Step 6: Post-Processing
    auto step6 = std::make_unique<TutorialStep>(
        "Add post-processing effects for cinematic quality:\n"
        "- Bloom: Glowing bright areas\n"
        "- Tone Mapping: HDR to LDR conversion\n"
        "- Anti-aliasing: Smooth edges\n"
        "- Ambient Occlusion: Contact shadows"
    );
    tutorial->AddStep(std::move(step6));
    
    // Step 7: Shader Editor
    auto step7 = std::make_unique<TutorialStep>(
        "For advanced users, open the Shader Editor:\n"
        "1. Create custom shaders with GLSL/HLSL\n"
        "2. Use the node-based shader graph\n"
        "3. Hot-reload shaders for instant feedback\n"
        "4. Debug with shader profiler"
    );
    tutorial->AddStep(std::move(step7));
    
    // Step 8: Performance Optimization
    auto step8 = std::make_unique<TutorialStep>(
        "Optimize rendering performance:\n"
        "- Use LODs (Level of Detail)\n"
        "- Implement occlusion culling\n"
        "- Batch similar draw calls\n"
        "- Use texture atlases\n"
        "- Profile with GPU debugger"
    );
    tutorial->AddStep(std::move(step8));
    
    // Step 9: Advanced Techniques
    auto step9 = std::make_unique<TutorialStep>(
        "Explore advanced rendering features:\n"
        "- Instanced rendering for many objects\n"
        "- Tessellation for detailed surfaces\n"
        "- Compute shaders for GPU calculations\n"
        "- Multi-pass rendering effects"
    );
    tutorial->AddStep(std::move(step9));
    
    return tutorial;
}

// =============================================================================
// PHYSICS TUTORIAL
// =============================================================================

std::unique_ptr<Tutorial> HelpContentFactory::CreatePhysicsTutorial() {
    auto tutorial = std::make_unique<Tutorial>("physics_tutorial", "Physics Simulation in ECScope");
    tutorial->SetCategory(HelpCategory::Physics);
    tutorial->SetEstimatedTime(20);
    tutorial->m_description = "Master physics simulation, collision detection, and dynamics";
    tutorial->m_targetLevel = UserLevel::Intermediate;
    
    // Step 1: Physics Overview
    auto step1 = std::make_unique<TutorialStep>(
        "ECScope's physics engine provides:\n"
        "- Rigid body dynamics\n"
        "- Collision detection and response\n"
        "- Joints and constraints\n"
        "- Triggers and sensors\n"
        "- Continuous collision detection"
    );
    tutorial->AddStep(std::move(step1));
    
    // Step 2: Rigid Bodies
    auto step2 = std::make_unique<TutorialStep>(
        "Add a Rigidbody component to make objects physically simulated:\n"
        "- Mass: Object weight\n"
        "- Drag: Air resistance\n"
        "- Angular Drag: Rotation resistance\n"
        "- Gravity Scale: Gravity multiplier"
    );
    tutorial->AddStep(std::move(step2));
    
    // Step 3: Colliders
    auto step3 = std::make_unique<TutorialStep>(
        "Colliders define the physical shape:\n"
        "- Box: Rectangular shape\n"
        "- Sphere: Round shape\n"
        "- Capsule: Pill shape\n"
        "- Mesh: Complex shape from model\n\n"
        "Add a collider to your object now."
    );
    step3->SetHint("Use simple colliders for better performance");
    tutorial->AddStep(std::move(step3));
    
    // Step 4: Collision Layers
    auto step4 = std::make_unique<TutorialStep>(
        "Configure collision layers to control what collides:\n"
        "1. Open Physics Settings\n"
        "2. Define layer names\n"
        "3. Set layer collision matrix\n"
        "4. Assign objects to layers"
    );
    tutorial->AddStep(std::move(step4));
    
    // Step 5: Forces and Impulses
    auto step5 = std::make_unique<TutorialStep>(
        "Apply forces to move objects:\n"
        "- AddForce: Gradual acceleration\n"
        "- AddImpulse: Instant velocity change\n"
        "- AddTorque: Rotation force\n\n"
        "Try different force modes in play mode."
    );
    tutorial->AddStep(std::move(step5));
    
    // Step 6: Joints
    auto step6 = std::make_unique<TutorialStep>(
        "Connect objects with joints:\n"
        "- Fixed: No relative movement\n"
        "- Hinge: Door-like rotation\n"
        "- Spring: Elastic connection\n"
        "- Slider: Linear movement"
    );
    tutorial->AddStep(std::move(step6));
    
    // Step 7: Triggers
    auto step7 = std::make_unique<TutorialStep>(
        "Use triggers for non-physical collision detection:\n"
        "1. Enable 'Is Trigger' on collider\n"
        "2. Implement OnTriggerEnter/Exit callbacks\n"
        "3. Use for zones, pickups, sensors"
    );
    tutorial->AddStep(std::move(step7));
    
    // Step 8: Physics Debugging
    auto step8 = std::make_unique<TutorialStep>(
        "Debug physics with visualization:\n"
        "- Show collision shapes\n"
        "- Display contact points\n"
        "- Visualize forces\n"
        "- Monitor performance metrics"
    );
    tutorial->AddStep(std::move(step8));
    
    return tutorial;
}

// =============================================================================
// AUDIO TUTORIAL
// =============================================================================

std::unique_ptr<Tutorial> HelpContentFactory::CreateAudioTutorial() {
    auto tutorial = std::make_unique<Tutorial>("audio_tutorial", "Audio System in ECScope");
    tutorial->SetCategory(HelpCategory::Audio);
    tutorial->SetEstimatedTime(15);
    tutorial->m_description = "Learn about 3D spatial audio, mixing, and effects";
    tutorial->m_targetLevel = UserLevel::Intermediate;
    
    // Step 1: Audio Overview
    auto step1 = std::make_unique<TutorialStep>(
        "ECScope's audio system features:\n"
        "- 3D spatial audio\n"
        "- Real-time mixing\n"
        "- Audio effects processing\n"
        "- Dynamic music system\n"
        "- Audio occlusion"
    );
    tutorial->AddStep(std::move(step1));
    
    // Step 2: Audio Sources
    auto step2 = std::make_unique<TutorialStep>(
        "Add an Audio Source component:\n"
        "- Audio Clip: Sound file to play\n"
        "- Volume: Playback volume\n"
        "- Pitch: Playback speed/pitch\n"
        "- Loop: Repeat playback\n"
        "- 3D Sound: Enable spatial audio"
    );
    tutorial->AddStep(std::move(step2));
    
    // Step 3: 3D Audio Settings
    auto step3 = std::make_unique<TutorialStep>(
        "Configure 3D audio parameters:\n"
        "- Min Distance: Full volume range\n"
        "- Max Distance: Silence range\n"
        "- Rolloff: Volume falloff curve\n"
        "- Doppler: Pitch shift with movement"
    );
    step3->SetHint("Use the 3D audio visualizer to see sound propagation");
    tutorial->AddStep(std::move(step3));
    
    // Step 4: Audio Mixer
    auto step4 = std::make_unique<TutorialStep>(
        "Open the Audio Mixer to control audio groups:\n"
        "1. Create mixer groups (Music, SFX, Voice)\n"
        "2. Route audio sources to groups\n"
        "3. Adjust group volumes\n"
        "4. Apply group effects"
    );
    tutorial->AddStep(std::move(step4));
    
    // Step 5: Audio Effects
    auto step5 = std::make_unique<TutorialStep>(
        "Add effects to enhance audio:\n"
        "- Reverb: Room acoustics\n"
        "- Echo: Delay effect\n"
        "- Distortion: Signal clipping\n"
        "- Low/High Pass: Frequency filtering"
    );
    tutorial->AddStep(std::move(step5));
    
    // Step 6: Audio Zones
    auto step6 = std::make_unique<TutorialStep>(
        "Create audio zones for environmental effects:\n"
        "1. Add Audio Zone component\n"
        "2. Define zone shape\n"
        "3. Configure reverb settings\n"
        "4. Set zone priority"
    );
    tutorial->AddStep(std::move(step6));
    
    // Step 7: Dynamic Music
    auto step7 = std::make_unique<TutorialStep>(
        "Implement dynamic music:\n"
        "- Create music layers\n"
        "- Set transition rules\n"
        "- Trigger based on game state\n"
        "- Synchronize tempo and beats"
    );
    tutorial->AddStep(std::move(step7));
    
    return tutorial;
}

// =============================================================================
// NETWORKING TUTORIAL
// =============================================================================

std::unique_ptr<Tutorial> HelpContentFactory::CreateNetworkingTutorial() {
    auto tutorial = std::make_unique<Tutorial>("networking_tutorial", "Multiplayer Networking");
    tutorial->SetCategory(HelpCategory::Networking);
    tutorial->SetEstimatedTime(30);
    tutorial->m_description = "Build multiplayer games with ECScope's networking system";
    tutorial->m_targetLevel = UserLevel::Advanced;
    tutorial->SetPrerequisites({"ecs_tutorial"});
    
    // Step 1: Networking Architecture
    auto step1 = std::make_unique<TutorialStep>(
        "ECScope networking architecture:\n"
        "- Client-Server model\n"
        "- Authoritative server\n"
        "- Client prediction\n"
        "- Lag compensation\n"
        "- Delta compression"
    );
    tutorial->AddStep(std::move(step1));
    
    // Step 2: Network Manager
    auto step2 = std::make_unique<TutorialStep>(
        "Configure the Network Manager:\n"
        "1. Set network mode (Host/Client/Server)\n"
        "2. Configure connection settings\n"
        "3. Set max players\n"
        "4. Define network tick rate"
    );
    tutorial->AddStep(std::move(step2));
    
    // Step 3: Network Components
    auto step3 = std::make_unique<TutorialStep>(
        "Add networking to entities:\n"
        "- NetworkObject: Makes entity network-aware\n"
        "- NetworkTransform: Syncs position/rotation\n"
        "- NetworkAnimator: Syncs animations\n"
        "- NetworkRigidbody: Syncs physics"
    );
    tutorial->AddStep(std::move(step3));
    
    // Step 4: Replication
    auto step4 = std::make_unique<TutorialStep>(
        "Configure replication:\n"
        "- Owner: Who controls the object\n"
        "- Authority: Who validates changes\n"
        "- Sync Rate: Update frequency\n"
        "- Reliability: Guaranteed delivery"
    );
    tutorial->AddStep(std::move(step4));
    
    // Step 5: RPCs
    auto step5 = std::make_unique<TutorialStep>(
        "Use Remote Procedure Calls (RPCs):\n"
        "- Client to Server: Send commands\n"
        "- Server to Client: Send events\n"
        "- Multicast: Broadcast to all\n\n"
        "RPCs enable custom network logic."
    );
    tutorial->AddStep(std::move(step5));
    
    // Step 6: Network Variables
    auto step6 = std::make_unique<TutorialStep>(
        "Synchronize data with Network Variables:\n"
        "1. Mark variables as [NetworkVar]\n"
        "2. Set permissions (Read/Write)\n"
        "3. Configure callbacks\n"
        "4. Handle conflicts"
    );
    tutorial->AddStep(std::move(step6));
    
    // Step 7: Lag Compensation
    auto step7 = std::make_unique<TutorialStep>(
        "Implement lag compensation:\n"
        "- Client-side prediction\n"
        "- Server reconciliation\n"
        "- Interpolation\n"
        "- Extrapolation\n\n"
        "This ensures smooth gameplay despite latency."
    );
    tutorial->AddStep(std::move(step7));
    
    // Step 8: Network Optimization
    auto step8 = std::make_unique<TutorialStep>(
        "Optimize network performance:\n"
        "- Reduce update frequency for distant objects\n"
        "- Use area of interest management\n"
        "- Compress data with delta encoding\n"
        "- Profile with network analyzer"
    );
    tutorial->AddStep(std::move(step8));
    
    return tutorial;
}

// =============================================================================
// ASSET PIPELINE TUTORIAL
// =============================================================================

std::unique_ptr<Tutorial> HelpContentFactory::CreateAssetPipelineTutorial() {
    auto tutorial = std::make_unique<Tutorial>("asset_pipeline", "Asset Pipeline Management");
    tutorial->SetCategory(HelpCategory::Assets);
    tutorial->SetEstimatedTime(15);
    tutorial->m_description = "Master asset importing, processing, and optimization";
    tutorial->m_targetLevel = UserLevel::Beginner;
    
    // Step 1: Asset Browser
    auto step1 = std::make_unique<TutorialStep>(
        "The Asset Browser is your file management center:\n"
        "- Navigate folder structure\n"
        "- Preview assets\n"
        "- Search and filter\n"
        "- Drag and drop to use assets"
    );
    step1->SetHighlight(ImVec2(0, 100), ImVec2(300, 800));
    tutorial->AddStep(std::move(step1));
    
    // Step 2: Importing Assets
    auto step2 = std::make_unique<TutorialStep>(
        "Import assets into your project:\n"
        "1. Drag files into Asset Browser\n"
        "2. Or use Import menu\n"
        "3. Configure import settings\n"
        "4. Assets are automatically processed"
    );
    step2->SetHint("ECScope supports common formats: FBX, OBJ, PNG, JPG, WAV, MP3");
    tutorial->AddStep(std::move(step2));
    
    // Step 3: Import Settings
    auto step3 = std::make_unique<TutorialStep>(
        "Customize import settings per asset type:\n"
        "- Textures: Resolution, compression, mipmaps\n"
        "- Models: Scale, normals, tangents\n"
        "- Audio: Sample rate, compression\n\n"
        "Right-click an asset and select 'Import Settings'."
    );
    tutorial->AddStep(std::move(step3));
    
    // Step 4: Asset References
    auto step4 = std::make_unique<TutorialStep>(
        "ECScope uses smart asset references:\n"
        "- Assets are referenced by GUID\n"
        "- Automatic dependency tracking\n"
        "- Missing asset detection\n"
        "- Hot-reload on changes"
    );
    tutorial->AddStep(std::move(step4));
    
    // Step 5: Asset Optimization
    auto step5 = std::make_unique<TutorialStep>(
        "Optimize assets for performance:\n"
        "- Texture atlasing\n"
        "- Model LODs\n"
        "- Audio compression\n"
        "- Asset bundling\n\n"
        "Use the Asset Optimizer tool for automatic optimization."
    );
    tutorial->AddStep(std::move(step5));
    
    // Step 6: Asset Variants
    auto step6 = std::make_unique<TutorialStep>(
        "Create platform-specific asset variants:\n"
        "1. Define quality tiers\n"
        "2. Set platform overrides\n"
        "3. Configure automatic selection\n"
        "4. Test with platform simulator"
    );
    tutorial->AddStep(std::move(step6));
    
    return tutorial;
}

// =============================================================================
// DEBUGGING TUTORIAL
// =============================================================================

std::unique_ptr<Tutorial> HelpContentFactory::CreateDebuggingTutorial() {
    auto tutorial = std::make_unique<Tutorial>("debugging_tutorial", "Debugging and Profiling");
    tutorial->SetCategory(HelpCategory::Debugging);
    tutorial->SetEstimatedTime(20);
    tutorial->m_description = "Master debugging tools and performance profiling";
    tutorial->m_targetLevel = UserLevel::Intermediate;
    
    // Step 1: Debug Tools Overview
    auto step1 = std::make_unique<TutorialStep>(
        "ECScope provides comprehensive debugging tools:\n"
        "- Performance Profiler\n"
        "- Memory Analyzer\n"
        "- Frame Debugger\n"
        "- Console Commands\n"
        "- Visual Debugging"
    );
    tutorial->AddStep(std::move(step1));
    
    // Step 2: Performance Profiler
    auto step2 = std::make_unique<TutorialStep>(
        "Open the Performance Profiler (F12):\n"
        "1. Start profiling session\n"
        "2. Perform actions to profile\n"
        "3. Stop and analyze results\n"
        "4. Identify bottlenecks in flame graph"
    );
    step2->SetHint("Focus on the tallest bars in the flame graph");
    tutorial->AddStep(std::move(step2));
    
    // Step 3: Memory Analysis
    auto step3 = std::make_unique<TutorialStep>(
        "Analyze memory usage:\n"
        "- Take memory snapshots\n"
        "- Compare snapshots for leaks\n"
        "- View allocation callstacks\n"
        "- Track object lifetimes"
    );
    tutorial->AddStep(std::move(step3));
    
    // Step 4: Frame Debugger
    auto step4 = std::make_unique<TutorialStep>(
        "Debug rendering with Frame Debugger:\n"
        "1. Capture a frame\n"
        "2. Step through draw calls\n"
        "3. Inspect render targets\n"
        "4. Analyze shader performance"
    );
    tutorial->AddStep(std::move(step4));
    
    // Step 5: Console Commands
    auto step5 = std::make_unique<TutorialStep>(
        "Use console commands for debugging:\n"
        "- help: List all commands\n"
        "- stats: Show performance stats\n"
        "- show [category]: Toggle debug visualizations\n"
        "- exec [script]: Run debug scripts"
    );
    step5->SetHint("Press ~ to open the console");
    tutorial->AddStep(std::move(step5));
    
    // Step 6: Visual Debugging
    auto step6 = std::make_unique<TutorialStep>(
        "Enable visual debugging overlays:\n"
        "- Collision shapes\n"
        "- Navigation mesh\n"
        "- Light bounds\n"
        "- Audio ranges\n"
        "- Network statistics"
    );
    tutorial->AddStep(std::move(step6));
    
    // Step 7: Logging
    auto step7 = std::make_unique<TutorialStep>(
        "Effective logging strategies:\n"
        "- Use log levels (Error, Warning, Info, Debug)\n"
        "- Add contextual information\n"
        "- Filter by categories\n"
        "- Export logs for analysis"
    );
    tutorial->AddStep(std::move(step7));
    
    // Step 8: Performance Optimization
    auto step8 = std::make_unique<TutorialStep>(
        "Common optimization techniques:\n"
        "- Reduce draw calls with batching\n"
        "- Use object pooling\n"
        "- Optimize Update() calls\n"
        "- Profile before optimizing\n\n"
        "Remember: Measure, don't guess!"
    );
    tutorial->AddStep(std::move(step8));
    
    return tutorial;
}

// =============================================================================
// PLUGIN TUTORIAL
// =============================================================================

std::unique_ptr<Tutorial> HelpContentFactory::CreatePluginTutorial() {
    auto tutorial = std::make_unique<Tutorial>("plugin_tutorial", "Creating and Using Plugins");
    tutorial->SetCategory(HelpCategory::Plugins);
    tutorial->SetEstimatedTime(25);
    tutorial->m_description = "Extend ECScope with custom plugins";
    tutorial->m_targetLevel = UserLevel::Advanced;
    
    // Step 1: Plugin System Overview
    auto step1 = std::make_unique<TutorialStep>(
        "ECScope's plugin system allows extending the engine:\n"
        "- Add custom components\n"
        "- Create new tools\n"
        "- Integrate third-party libraries\n"
        "- Share functionality"
    );
    tutorial->AddStep(std::move(step1));
    
    // Step 2: Plugin Manager
    auto step2 = std::make_unique<TutorialStep>(
        "Open the Plugin Manager to:\n"
        "- Browse installed plugins\n"
        "- Enable/disable plugins\n"
        "- Configure plugin settings\n"
        "- Access plugin documentation"
    );
    step2->SetHighlight(ImVec2(800, 100), ImVec2(600, 700));
    tutorial->AddStep(std::move(step2));
    
    // Step 3: Installing Plugins
    auto step3 = std::make_unique<TutorialStep>(
        "Install plugins from the marketplace:\n"
        "1. Browse available plugins\n"
        "2. Check compatibility\n"
        "3. Read reviews and documentation\n"
        "4. Click Install"
    );
    tutorial->AddStep(std::move(step3));
    
    // Step 4: Creating a Plugin
    auto step4 = std::make_unique<TutorialStep>(
        "Create your first plugin:\n"
        "1. File > New > Plugin Project\n"
        "2. Choose plugin template\n"
        "3. Configure plugin metadata\n"
        "4. Implement plugin interface"
    );
    tutorial->AddStep(std::move(step4));
    
    // Step 5: Plugin API
    auto step5 = std::make_unique<TutorialStep>(
        "Use the Plugin API to:\n"
        "- Register custom components\n"
        "- Add menu items\n"
        "- Create tool windows\n"
        "- Hook into engine events"
    );
    step5->SetHint("Check API documentation for available interfaces");
    tutorial->AddStep(std::move(step5));
    
    // Step 6: Plugin Resources
    auto step6 = std::make_unique<TutorialStep>(
        "Manage plugin resources:\n"
        "- Bundle assets with plugin\n"
        "- Load plugin-specific configs\n"
        "- Handle plugin data paths\n"
        "- Version compatibility"
    );
    tutorial->AddStep(std::move(step6));
    
    // Step 7: Testing Plugins
    auto step7 = std::make_unique<TutorialStep>(
        "Test your plugin:\n"
        "1. Use Plugin Test Harness\n"
        "2. Enable hot-reload for development\n"
        "3. Check different engine versions\n"
        "4. Profile performance impact"
    );
    tutorial->AddStep(std::move(step7));
    
    // Step 8: Publishing Plugins
    auto step8 = std::make_unique<TutorialStep>(
        "Share your plugin:\n"
        "1. Package plugin files\n"
        "2. Write documentation\n"
        "3. Create marketplace listing\n"
        "4. Set pricing (if commercial)\n"
        "5. Submit for review"
    );
    tutorial->AddStep(std::move(step8));
    
    return tutorial;
}

// =============================================================================
// SCRIPTING TUTORIAL
// =============================================================================

std::unique_ptr<Tutorial> HelpContentFactory::CreateScriptingTutorial() {
    auto tutorial = std::make_unique<Tutorial>("scripting_tutorial", "Scripting in ECScope");
    tutorial->SetCategory(HelpCategory::Scripting);
    tutorial->SetEstimatedTime(30);
    tutorial->m_description = "Write game logic with ECScope's scripting system";
    tutorial->m_targetLevel = UserLevel::Intermediate;
    
    // Step 1: Scripting Overview
    auto step1 = std::make_unique<TutorialStep>(
        "ECScope supports multiple scripting approaches:\n"
        "- C++ for maximum performance\n"
        "- Lua for rapid prototyping\n"
        "- Visual scripting for designers\n"
        "- Hot-reload for iteration"
    );
    tutorial->AddStep(std::move(step1));
    
    // Step 2: Script Editor
    auto step2 = std::make_unique<TutorialStep>(
        "Open the Script Editor:\n"
        "- Syntax highlighting\n"
        "- Auto-completion\n"
        "- Error checking\n"
        "- Integrated debugger\n\n"
        "Create a new script: File > New Script"
    );
    step2->SetHighlight(ImVec2(400, 100), ImVec2(1000, 800));
    tutorial->AddStep(std::move(step2));
    
    // Step 3: Component Scripts
    auto step3 = std::make_unique<TutorialStep>(
        "Create a component script:\n"
        "```cpp\n"
        "class PlayerController : public Component {\n"
        "    void Start() { }\n"
        "    void Update(float dt) { }\n"
        "};\n"
        "```\n"
        "Attach to entities like any component."
    );
    tutorial->AddStep(std::move(step3));
    
    // Step 4: Event Handling
    auto step4 = std::make_unique<TutorialStep>(
        "Handle engine events:\n"
        "- OnCollisionEnter/Exit\n"
        "- OnTriggerEnter/Exit\n"
        "- OnEnable/Disable\n"
        "- OnDestroy\n\n"
        "Events enable reactive gameplay logic."
    );
    tutorial->AddStep(std::move(step4));
    
    // Step 5: Accessing Components
    auto step5 = std::make_unique<TutorialStep>(
        "Access other components:\n"
        "```cpp\n"
        "auto transform = GetComponent<Transform>();\n"
        "auto rb = GetComponent<Rigidbody>();\n"
        "rb->AddForce(Vector3::up * 10);\n"
        "```"
    );
    tutorial->AddStep(std::move(step5));
    
    // Step 6: Hot Reload
    auto step6 = std::make_unique<TutorialStep>(
        "Enable hot reload for rapid iteration:\n"
        "1. Enable 'Hot Reload' in settings\n"
        "2. Edit and save scripts\n"
        "3. Changes apply immediately\n"
        "4. State is preserved when possible"
    );
    step6->SetHint("Hot reload works best with Lua scripts");
    tutorial->AddStep(std::move(step6));
    
    // Step 7: Debugging Scripts
    auto step7 = std::make_unique<TutorialStep>(
        "Debug your scripts:\n"
        "- Set breakpoints (F9)\n"
        "- Step through code (F10)\n"
        "- Inspect variables\n"
        "- View call stack\n"
        "- Use Debug.Log() for output"
    );
    tutorial->AddStep(std::move(step7));
    
    // Step 8: Visual Scripting
    auto step8 = std::make_unique<TutorialStep>(
        "Try visual scripting for logic without code:\n"
        "1. Create Visual Script asset\n"
        "2. Add nodes from palette\n"
        "3. Connect node pins\n"
        "4. Set node properties\n"
        "5. Attach to entities"
    );
    tutorial->AddStep(std::move(step8));
    
    // Step 9: Performance Tips
    auto step9 = std::make_unique<TutorialStep>(
        "Script performance best practices:\n"
        "- Cache component references\n"
        "- Avoid Update() when possible\n"
        "- Use events over polling\n"
        "- Pool frequently created objects\n"
        "- Profile script performance"
    );
    tutorial->AddStep(std::move(step9));
    
    return tutorial;
}

} // namespace ecscope::gui::help