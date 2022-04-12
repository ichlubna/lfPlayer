import bpy

class LFPanel(bpy.types.Panel):
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_context = "objectmode"
    bl_category = "LF"
    bl_label = "Exports the LF projection"

    def draw(self, context):
        col = self.layout.column(align=True)
        col.prop(context.scene, "LFImages")
        col.prop(context.scene, "LFCoords")
        col.prop(context.scene, "LFRange")
        col.prop(context.scene, "LFDensity")
        col.operator("lf.render", text="Render")

class LFRender(bpy.types.Operator):
    """ Renders the LF structure
    """
    bl_idname = "lf.render"
    bl_label = "Render"
    
    coordsMaterial = None

    def createCoordsMaterial(self):
        materialName = "coordsMaterial"
        material = bpy.data.materials.get(materialName) or bpy.data.materials.new(name=materialName)
        material.use_nodes = True
        material.node_tree.nodes.clear()
        
        materialOut = material.node_tree.nodes.new('ShaderNodeOutputMaterial')
        materialOut.location[0]=400  
        emission = material.node_tree.nodes.new('ShaderNodeEmission')
        emission.location[0]=200   
        divide = material.node_tree.nodes.new('ShaderNodeVectorMath')
        divide.operation="DIVIDE"
        divide.location[0]=0  
        planes = material.node_tree.nodes.new('ShaderNodeCombineXYZ')
        planes.location[0]=-200
        planes.location[1]=-200
        multiply = material.node_tree.nodes.new('ShaderNodeVectorMath')
        multiply.operation="MULTIPLY"
        multiply.location[0]=-200  
        cameraData = material.node_tree.nodes.new('ShaderNodeCameraData')
        cameraData.location[0]=-400

        clip = bpy.context.scene.camera.data.clip_end
        planes.inputs[0].default_value = clip
        planes.inputs[1].default_value = clip
        planes.inputs[2].default_value = clip

        material.node_tree.links.new(multiply.inputs[0], cameraData.outputs[0])
        material.node_tree.links.new(multiply.inputs[1], cameraData.outputs[2])
        material.node_tree.links.new(divide.inputs[0], multiply.outputs[0])
        material.node_tree.links.new(divide.inputs[1], planes.outputs[0])
        material.node_tree.links.new(emission.inputs[0], divide.outputs[0])
        material.node_tree.links.new(materialOut.inputs[0], emission.outputs[0])
        
        self.coordsMaterial = material

    def render(self, context):
        renderInfo = bpy.context.scene.render
        originalPath = (renderInfo.filepath + '.')[:-1]
        camera = bpy.context.scene.camera     
        
        step = context.scene.LFRange/context.scene.LFDensity
        for i in range(context.scene.LFDensity):
            camPos = -context.scene.LFRange*0.5+step
            camera.delta_location[0] = camPos #TODO get the vector perpendicular to direction
            renderInfo.filepath = originalPath+"/"+context.scene.LFImages+"/"+f'{i:05d}'+".png"
            renderInfo.image_settings.file_format = 'PNG'
            bpy.ops.render.render( write_still=True )  
            renderInfo.filepath = originalPath+"/"+context.scene.LFCoords+"/"+f'{i:05d}'+".exr"
            renderInfo.image_settings.file_format = 'OPEN_EXR'
            context.window.view_layer.material_override = self.coordsMaterial
            bpy.ops.render.render( write_still=True )  
            context.window.view_layer.material_override = None
        
        renderInfo.filepath = originalPath
        #export info cam    

    def invoke(self, context, event):
        self.createCoordsMaterial()
        self.render(context)
        return {"FINISHED"}

def register():
    bpy.utils.register_class(LFPanel)
    bpy.utils.register_class(LFRender)
    bpy.types.Scene.LFImages = bpy.props.StringProperty(name="Image dir name", description="Where to store color images", default="color")
    bpy.types.Scene.LFCoords = bpy.props.StringProperty(name="Coords dir name", description="Where to store coordinate images", default="coords")
    bpy.types.Scene.LFRange = bpy.props.FloatProperty(name="Range", description="The total width of the LF (camera positions)", default=1.0)
    bpy.types.Scene.LFDensity = bpy.props.IntProperty(name="Density", description="The total number of the views", min=1, default=32)
    
def unregister():
    bpy.utils.unregister_class(LFPanel)
    bpy.utils.unregister_class(LFRender)
    
if __name__ == "__main__" :
    register()        