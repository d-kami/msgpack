package jp.ac.tsukuba.cs.softlab.translator;

import java.util.Map;

import java.io.File;
import java.io.InputStream;
import java.io.Writer;
import java.io.StringWriter;
import java.io.FileWriter;
import java.io.FileInputStream;
import java.io.OutputStreamWriter;
import java.io.ByteArrayInputStream;
import java.io.IOException;

import jp.ac.tsukuba.cs.softlab.msgpack.Schema;
import jp.ac.tsukuba.cs.softlab.msgpack.schema.ClassGenerator;

import org.eclipse.core.resources.IncrementalProjectBuilder;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.resources.IResourceDelta;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.resources.IResourceVisitor;
import org.eclipse.core.resources.IResourceDeltaVisitor;

public class ClassTranslator extends IncrementalProjectBuilder{
	public static final String BUILDER_ID = "ClassTranslator.ClassTranslator";
	
    protected IProject[] build(int kind, Map args, IProgressMonitor monitor){
        if (kind == FULL_BUILD) {
            fullBuild(monitor);
        } else {
            IResourceDelta delta = getDelta(getProject());
            if (delta == null) {
                fullBuild(monitor);
            } else {
                incrementalBuild(delta, monitor);
            }
        }
        return null;
    }
    
    private void fullBuild(IProgressMonitor monitor){
        try{
            getProject().accept(new ClassResourceVisitor());
        }catch(CoreException e){
        	e.printStackTrace();
        }
    }

    private void incrementalBuild(IResourceDelta delta, IProgressMonitor monitor){ // … （3）
        try {
            delta.accept(new ClassDeltaVisitor());
        } catch (CoreException e) {
            e.printStackTrace();
        }
    }
    
    private static void translate(IResource resource){
    	if(resource instanceof IFile && resource.getName().endsWith(".mpack")){
    		IFile file = (IFile)resource;
    		
    		try{
    			String javaSource = ClassTranslator.toJavaSource(file.getContents());
              ByteArrayInputStream in = new ByteArrayInputStream(javaSource.getBytes());
    			
    			String path = file.getProjectRelativePath().toString();
    			path = ClassTranslator.toJavaFileName(path);
    			
    			IProject project = resource.getProject();
    			IFile sourceFile = project.getFile(path);
    			sourceFile.create(in, true, null);
    		}catch(Exception e){
    			e.printStackTrace();
    		}
        }
    }
    
    private static String toJavaSource(InputStream input) throws IOException{
        StringWriter writer = new StringWriter();
        Schema schema = Schema.parse(input);
        ClassGenerator.write(schema, writer);
        writer.flush();
        
        return writer.toString();
    }
    
    private static String toJavaFileName(String filename){    	
    	int lastDotPos = filename.lastIndexOf('.');

    	if(lastDotPos == -1){
    		return filename;
       }else if (lastDotPos == 0){
    		return filename;
       }else{
    		return filename.substring(0, lastDotPos) + ".java";
        }
    }
    
    private static class ClassResourceVisitor implements IResourceVisitor {
        public boolean visit(IResource resource) throws CoreException {
            ClassTranslator.translate(resource);
            return true;
         }
    }
    
    private static class ClassDeltaVisitor implements IResourceDeltaVisitor {
        public boolean visit(IResourceDelta delta) throws CoreException {
    	     IResource resource = delta.getResource();
    	     int kind = delta.getKind();

            if(kind == IResourceDelta.ADDED || kind == IResourceDelta.CHANGED){
                ClassTranslator.translate(resource);
              }
           
            return true;
        }
    }
}