#ifndef ZTRACE_CREATE_AMBIENT_LIGHTS_H
#define ZTRACE_CREATE_AMBIENT_LIGHTS_H

#include "Types.h"
#include "TraceableList.h"
#include "Light.h"
#include "LightList.h"
#include "Ray.h"

namespace ztrace {

    // \todo handle glass
    // \todo exceptions!
    //
    bool getNewLightPosition( Light const & current, 
            Traceable const & world, 
            TraceData & traceData, 
            Ray & rayOut ) {
        Size counter = 0;
        do {
          current.emitForwardRay( rayOut );
          ++counter;
        } while( ! world.hit( rayOut, 0.01, current.intensity(), traceData) and counter < 20 );
        if( counter < 20 )
        {
            return true;
        }
        return false;
    }

    bool getIntensity( Light const & current, 
            TraceData const & newPosition, 
            Vector & intensityOut ) {
        ShadowRay backward;
        if( current.getShadowRay( newPosition.point, intensityOut, backward ) ) {
            return true;
        }
        return false;
    }
    bool checkIntensity( Vector const & intensity, Real const & minIntensity ) {
        if( intensity.len() > minIntensity ) {
            return true;
        }
        return false;
    }

    bool addFurtherLightsToList( LightList & list, Light const & current, Traceable const & world, Size maxDepth, Real minIntensity ) {
        TraceData traceData;
        Vector    intensity;
        Real      distance;
        Ray       forwardRay;
        if( getNewLightPosition( current, world, traceData, forwardRay ) ){
            if( getIntensity( current, traceData, intensity ) 
                    and checkIntensity( intensity, minIntensity ) ){
                Vector attenuation;
                Ray    scatteredRay;
                if( ( traceData.material->getGloss().hasSpecular() 
                            or traceData.material->getGloss().hasTransmission() ) ) {
                    if( traceData.material->scatterView( forwardRay, traceData, attenuation, scatteredRay ) ) {
                        std::shared_ptr<Light> newLight;
                        newLight = std::make_shared<LightChildSpot>( scatteredRay.origin(), 
                                attenuation * intensity, 
                                current.intensity( ), 
                                scatteredRay.direction(),
                                traceData.material->fuzz(),
                                traceData.positionOnRay
                                );
                        list.add( newLight );
                        addFurtherLightsToList( list, *newLight, world, maxDepth-1, minIntensity );
                    }
                } 
                if( traceData.material->getGloss().hasDiffusive() ) {
                    std::shared_ptr<Light> newLight;
                    newLight = std::make_shared<LightChildPointSource>( traceData.point, 
                            traceData.material->getGloss().albedo() * intensity / 2. / 3.1415, 
                            current.intensity( ) / 2. / 3.1416, 
                            traceData.positionOnRay );
                    list.add( newLight );
                } 
            }
        }
        return false;
    }
    bool addSpotToList( LightList & list, 
            Light const & current, 
            Traceable const & world, 
            Size maxDepth, 
            Real minIntensity ) {
        TraceData traceData;
        Vector    intensity;
        Real      distance;
        Ray       forwardRay;
        if( getNewLightPosition( current, world, traceData, forwardRay ) ){
            if( getIntensity( current, traceData, intensity ) 
                    and checkIntensity( intensity, minIntensity ) ){
                Vector attenuation;
                Ray    scatteredRay;
                if( ( traceData.material->getGloss().hasSpecular() 
                            or traceData.material->getGloss().hasTransmission() ) ) {
                    if( traceData.material->scatterView( forwardRay, traceData, attenuation, scatteredRay ) ) {
                        std::shared_ptr<Light> newLight;
                        newLight = std::make_shared<LightChildSpot>( scatteredRay.origin(), 
                                attenuation * intensity, 
                                current.intensity( ), 
                                scatteredRay.direction(),
                                traceData.material->fuzz(),
                                traceData.positionOnRay
                                );
                        list.add( newLight );
                        addFurtherLightsToList( list, *newLight, world, maxDepth-1, minIntensity );
                        return true;
                    }
                } 
            }
        }
        return false;
    }

    LightList createAmbientLights( LightList const & oldLights, 
            Traceable const & world, 
            Size numberPerLight, 
            Size maxDepth = 20, 
            Real minIntensity = 0.1 ) 
    {
        LightList newLights;
        
        for( auto light: oldLights ) {
            newLights.add( light );
            Light & current = *light;
            for( Size i = 0; i < numberPerLight; i++ )
            {
                Size counter = 0;
                while( ! addSpotToList( newLights, current, world, maxDepth, minIntensity ) 
                        and counter < 5 ){
                    ++counter;
                }
                if( counter == 5 ) {
                    addFurtherLightsToList( newLights, current, world, maxDepth, minIntensity );
                }
            }
        }
        return newLights;
    }
}

#endif
